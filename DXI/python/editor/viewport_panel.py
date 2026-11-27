"""The viewport with its header bar, as Unreal's editor has it: the transform tools and the Show menu above the
view, the frame rate beside them.

The header is a bar above the viewport and not an overlay on it. The engine draws into a native window of its
own, and a Qt widget can not be painted on top of one.

The Show menu switches what is drawn on and off, and its last entry, Options..., opens the window with how each
of those looks, one tab per entry, see options_dialog.py.

The tangent frame and the origin axes apply to the whole scene: the engine draws them for the objects that are
flagged, and the flag is not something to ask a user to set object by object, so while an overlay is on every
object is flagged, and one that appears later gets the flag too.
"""

from PySide6.QtCore import Qt
from PySide6.QtGui import QAction
from PySide6.QtWidgets import QFrame, QHBoxLayout, QLabel, QMenu, QToolButton, QVBoxLayout, QWidget

from options_dialog import OptionsDialog
from viewport import ViewportWidget
from widgets import set_quietly


class ViewportPanel(QWidget):
    def __init__(self, dxi, engine_args="", parent=None):
        super().__init__(parent)
        self._dxi = dxi
        self._renderer = None
        self._flagged_for = None  # (objects, tangent frame on, origin axes on) the flags were last set for

        self.viewport = ViewportWidget(dxi, engine_args)
        self._options = OptionsDialog(dxi, self)

        self._move = self._tool_button("Move", "Move tool  (1)", checkable=True)
        self._rotate = self._tool_button("Rotate", "Rotate tool  (2)", checkable=True)
        self._move.setAutoExclusive(True)
        self._rotate.setAutoExclusive(True)
        self._move.clicked.connect(lambda: self._set_gizmo("mode", dxi.TransformMode.Move))
        self._rotate.clicked.connect(lambda: self._set_gizmo("mode", dxi.TransformMode.Rotate))

        # World or the object's own axes, one button that says which it is on
        self._space = self._tool_button("World", "Gizmo axes: world or the object's own", checkable=True)
        self._space.toggled.connect(self._on_space_toggled)

        self._show = self._tool_button("Show  \u25be", "What the viewport draws on top of the scene")
        self._show.setMenu(self._build_show_menu())
        self._show.setPopupMode(QToolButton.InstantPopup)

        self._fps = QLabel("")
        self._fps.setObjectName("Dim")
        self.viewport.fpsChanged.connect(lambda fps: self._fps.setText(f"{fps} fps"))

        header = QFrame()
        header.setObjectName("ViewportHeader")
        row = QHBoxLayout(header)
        row.setContentsMargins(6, 3, 6, 3)
        row.setSpacing(2)
        row.addWidget(self._move)
        row.addWidget(self._rotate)
        row.addSpacing(8)
        row.addWidget(self._space)
        row.addStretch(1)
        row.addWidget(self._show)
        row.addWidget(self._fps)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(header)
        layout.addWidget(self.viewport, 1)

    @staticmethod
    def _tool_button(text, tooltip, checkable=False):
        button = QToolButton()
        button.setText(text)
        button.setToolTip(tooltip)
        button.setCheckable(checkable)
        # Keys belong to the viewport: a click here must not take them away from the camera
        button.setFocusPolicy(Qt.NoFocus)
        return button

    def _build_show_menu(self):
        menu = QMenu(self)

        self._show_actions = []  # (action, settings of the overlay it switches)
        for text, resolve in (("Selection Highlight", lambda r: r.selection),
                              ("Transform Gizmo", lambda r: r.gizmo),
                              ("Tangent Frame", lambda r: r.tangent_frame),
                              ("Origin Axes", lambda r: r.axes)):
            action = QAction(text, menu, checkable=True)
            action.toggled.connect(lambda checked, resolve=resolve: self._set_enabled(resolve, checked))
            menu.addAction(action)
            self._show_actions.append((action, resolve))

        # Last in the list, apart from the switches
        menu.addSeparator()
        menu.addAction("Options...", self._options.open)

        return menu

    # Engine and header

    def set_renderer(self, renderer):
        self._renderer = renderer
        self._options.set_renderer(renderer)
        self._flagged_for = None
        self.refresh()

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        gizmo = renderer.gizmo
        set_quietly(self._move, self._move.setChecked, gizmo.mode == self._dxi.TransformMode.Move)
        set_quietly(self._rotate, self._rotate.setChecked, gizmo.mode == self._dxi.TransformMode.Rotate)

        local = gizmo.space == self._dxi.TransformSpace.Local
        set_quietly(self._space, self._space.setChecked, local)
        self._space.setText("Local" if local else "World")

        for action, resolve in self._show_actions:
            set_quietly(action, action.setChecked, resolve(renderer).enabled)

        self._options.refresh()

        # Caught here whoever switched an overlay, this menu or a tab of the Options window, and whenever an
        # object appeared since
        self._flag_objects()

    def _flag_objects(self):
        """Every object is flagged for an overlay while it is on, and for none while it is off"""
        renderer = self._renderer
        state = (renderer.object_count, renderer.tangent_frame.enabled, renderer.axes.enabled)
        if state == self._flagged_for:
            return

        _, tangent, axes = state
        for index in range(renderer.object_count):
            renderer.set_overlay_flags(index, tangent_frame=tangent, origin_axes=axes)

        self._flagged_for = state

    def _set_gizmo(self, attr, value):
        if self._renderer is not None and self._renderer.alive:
            setattr(self._renderer.gizmo, attr, value)

    def _on_space_toggled(self, local):
        self._set_gizmo("space", self._dxi.TransformSpace.Local if local else self._dxi.TransformSpace.World)
        self._space.setText("Local" if local else "World")

    def _set_enabled(self, resolve, checked):
        if self._renderer is not None and self._renderer.alive:
            resolve(self._renderer).enabled = checked
            self._flag_objects()
