"""The widget the engine draws into.

The engine renders into a native window handle. A Qt widget that asks for a native window (WA_NativeWindow)
has one, and that child window is what the swap chain is made for. Qt must not paint on it as well, so
the widget claims the whole surface (WA_PaintOnScreen, no paint engine) and leaves it to the engine.

Everything runs on the GUI thread: the timer that draws frames, the input handlers and the panels that
read and edit the scene. The engine is not thread safe, and nothing here needs it to be.
"""

import ctypes
from ctypes import wintypes

from PySide6.QtCore import QTimer, Qt, Signal
from PySide6.QtWidgets import QWidget

_user32 = ctypes.WinDLL("user32")
_user32.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]

FRAME_INTERVAL_MS = 16  # about 60 frames a second, the engine has vsync off and would spin otherwise


def client_size(hwnd):
    """Size of a window's client area in physical pixels, which is what the swap chain has to match.

    Asked of Windows rather than worked out from the widget's size and the display scale, since Qt
    rounds that and the engine would then stretch the image by a pixel.
    """
    rect = wintypes.RECT()
    _user32.GetClientRect(hwnd, ctypes.byref(rect))
    return rect.right - rect.left, rect.bottom - rect.top


class ViewportWidget(QWidget):
    initialized = Signal()
    failed = Signal(str)
    selectionChanged = Signal(int)  # index of the selected object, -1 for none, whoever changed it
    fpsChanged = Signal(int)

    def __init__(self, dxi, engine_args="", parent=None):
        super().__init__(parent)
        self._dxi = dxi
        self._engine_args = engine_args
        self.renderer = None

        # A native window of its own, which Qt leaves unpainted
        self.setAttribute(Qt.WA_NativeWindow)
        self.setAttribute(Qt.WA_PaintOnScreen)
        self.setAttribute(Qt.WA_NoSystemBackground)
        self.setAttribute(Qt.WA_OpaquePaintEvent)
        self.setFocusPolicy(Qt.StrongFocus)  # keys drive the camera, so a click has to give focus
        self.setMouseTracking(True)          # the gizmo lights the handle under the cursor with no button down
        self.setMinimumSize(320, 200)

        self._timer = QTimer(self)
        self._timer.setTimerType(Qt.PreciseTimer)
        self._timer.timeout.connect(self._tick)

        self._started = False
        self._looking = False          # right button held, the mouse turns the camera
        self._last_look = (0, 0)
        self._held_keys = set()
        self._last_selected = -1
        self._frames = 0

        self._fps_timer = QTimer(self)
        self._fps_timer.timeout.connect(self._report_fps)
        self._fps_timer.start(1000)

    # Qt would otherwise draw into the surface the swap chain presents to
    def paintEngine(self):
        return None

    @property
    def has_engine(self):
        return self.renderer is not None and self.renderer.alive

    def shutdown(self):
        """Stops drawing and waits for the GPU. Has to happen while the window still exists."""
        self._timer.stop()
        self._fps_timer.stop()
        if self.renderer is not None:
            self.renderer.shutdown()

    # Lifetime

    def showEvent(self, event):
        super().showEvent(event)
        if not self._started:
            self._started = True
            # After the event that shows the widget, so the native window has its final size
            QTimer.singleShot(0, self._create_renderer)

    def _create_renderer(self):
        hwnd = int(self.winId())
        width, height = client_size(hwnd)

        try:
            # Compiles the shaders and loads the scene, a few seconds with the window frozen
            self.renderer = self._dxi.Renderer(hwnd, max(width, 1), max(height, 1), self._engine_args)
        except Exception as error:  # a failed D3D12 call arrives as a RuntimeError
            self.failed.emit(str(error))
            return

        self._last_selected = self.renderer.selected
        self._timer.start(FRAME_INTERVAL_MS)
        self.initialized.emit()

    def _tick(self):
        renderer = self.renderer
        if renderer is None or not renderer.alive:
            return

        # Follows the window here rather than in resizeEvent: a drag sends a resize per pixel and
        # the engine waits for the GPU on each, this way it is once per frame at most
        width, height = client_size(int(self.winId()))
        if (width, height) != (renderer.width, renderer.height):
            renderer.resize(width, height)

        renderer.tick()
        self._frames += 1

        # A click in the view or the Escape key changes it behind the panels' back
        selected = renderer.selected
        if selected != self._last_selected:
            self._last_selected = selected
            self.selectionChanged.emit(selected)

    def _report_fps(self):
        self.fpsChanged.emit(self._frames)
        self._frames = 0

    # Input | Positions go to the engine in physical pixels, as its own window reports them

    def _pixels(self, event):
        position = event.position()
        ratio = self.devicePixelRatioF()
        return int(position.x() * ratio), int(position.y() * ratio)

    def mousePressEvent(self, event):
        self.setFocus()
        if not self.has_engine:
            return

        x, y = self._pixels(event)
        if event.button() == Qt.LeftButton:
            self.renderer.left_button_down(x, y)
        elif event.button() == Qt.RightButton:
            self._looking = True
            self._last_look = (x, y)

    def mouseMoveEvent(self, event):
        if not self.has_engine:
            return

        x, y = self._pixels(event)
        if self._looking:
            self.renderer.mouse_look(x - self._last_look[0], y - self._last_look[1])
        self._last_look = (x, y)

        # Also while turning the camera, as the executable does: a held gizmo handle needs it
        self.renderer.cursor_move(x, y)

    def mouseReleaseEvent(self, event):
        if not self.has_engine:
            return

        x, y = self._pixels(event)
        if event.button() == Qt.LeftButton:
            self.renderer.left_button_up(x, y)
        elif event.button() == Qt.RightButton:
            self._looking = False

    def keyPressEvent(self, event):
        # The native key is the Windows virtual key code the engine's camera is written against
        key = event.nativeVirtualKey()
        if event.isAutoRepeat() or not key or not self.has_engine:
            return

        self._held_keys.add(key)
        self.renderer.key_down(key)

    def keyReleaseEvent(self, event):
        key = event.nativeVirtualKey()
        if event.isAutoRepeat() or not key or not self.has_engine:
            return

        self._held_keys.discard(key)
        self.renderer.key_up(key)

    def focusOutEvent(self, event):
        # A key released while another widget had focus is never seen here, and the camera
        # would keep flying
        if self.has_engine:
            for key in self._held_keys:
                self.renderer.key_up(key)
        self._held_keys.clear()
        self._looking = False
        super().focusOutEvent(event)
