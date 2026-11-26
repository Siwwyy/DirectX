"""The editor window, laid out as Unreal's is: a menu bar over the viewport, the outliner above the details on the
right, and a status bar below. The viewport settings share a tab with the details, as World Settings does there."""

from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QAction
from PySide6.QtWidgets import QDockWidget, QMainWindow, QMessageBox, QScrollArea, QTabWidget, QWidget

from panels import DetailsPanel, OutlinerPanel, SettingsPanel
from viewport_panel import ViewportPanel

PANEL_REFRESH_MS = 100  # the engine changes things on its own, see panels.py
RIGHT_COLUMN_WIDTH = 400

HINT = ("Left click selects, drag a handle to move or rotate it (Ctrl snaps).  "
        "Hold the right button to look, W A S D to fly.  1 and 2 switch the tool, Esc deselects.")


class MainWindow(QMainWindow):
    def __init__(self, dxi, engine_args=""):
        super().__init__()
        self._dxi = dxi
        self.setWindowTitle(f"DXI Editor  |  dxi {dxi.__version__}")
        self.resize(1600, 900)
        self.setDockOptions(QMainWindow.AllowTabbedDocks | QMainWindow.AllowNestedDocks | QMainWindow.AnimatedDocks)

        # Tabs above their dock, Qt's default is below
        self.setTabPosition(Qt.AllDockWidgetAreas, QTabWidget.North)

        self.viewport_panel = ViewportPanel(dxi, engine_args)
        self.viewport = self.viewport_panel.viewport
        self.setCentralWidget(self.viewport_panel)

        self.outliner = OutlinerPanel()
        self.details = DetailsPanel(dxi)
        self.settings = SettingsPanel(dxi)

        outliner_dock = self._add_dock("Outliner", self.outliner)
        details_dock = self._add_dock("Details", self._scrolled(self.details))
        settings_dock = self._add_dock("Viewport Settings", self._scrolled(self.settings))

        # The tab names these two, a title bar under it would say the same thing twice
        for tabbed in (details_dock, settings_dock):
            tabbed.setTitleBarWidget(QWidget())

        # Outliner on top, details under it, settings in the details' tab
        self.addDockWidget(Qt.RightDockWidgetArea, outliner_dock)
        self.splitDockWidget(outliner_dock, details_dock, Qt.Vertical)
        self.tabifyDockWidget(details_dock, settings_dock)
        details_dock.raise_()
        self.resizeDocks([outliner_dock, details_dock], [260, 600], Qt.Vertical)
        self.resizeDocks([outliner_dock], [RIGHT_COLUMN_WIDTH], Qt.Horizontal)

        self._build_menus([outliner_dock, details_dock, settings_dock])
        self.statusBar().showMessage("Building the scene, the shaders are compiling...")

        self.viewport.initialized.connect(self._on_initialized)
        self.viewport.failed.connect(self._on_failed)
        self.viewport.selectionChanged.connect(self.outliner.show_selection)

        self._refresh_timer = QTimer(self)
        self._refresh_timer.timeout.connect(self._refresh_panels)

    # Layout

    def _add_dock(self, title, widget):
        dock = QDockWidget(title, self)
        dock.setObjectName(title)
        dock.setWidget(widget)
        return dock

    @staticmethod
    def _scrolled(widget):
        # Taller than a small window, so it scrolls rather than force the window to grow
        scroll = QScrollArea()
        scroll.setWidget(widget)
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        return scroll

    def _build_menus(self, docks):
        bar = self.menuBar()

        file_menu = bar.addMenu("&File")
        file_menu.addAction(self._action("E&xit", self.close, "Ctrl+Q"))

        edit_menu = bar.addMenu("&Edit")
        edit_menu.addAction(self._action("&Deselect All", self._deselect))

        window_menu = bar.addMenu("&Window")
        for dock in docks:
            window_menu.addAction(dock.toggleViewAction())

        help_menu = bar.addMenu("&Help")
        help_menu.addAction(self._action("&About", self._about))

    def _action(self, text, slot, shortcut=None):
        action = QAction(text, self)
        action.triggered.connect(slot)
        if shortcut:
            action.setShortcut(shortcut)
        return action

    # Slots

    def _on_initialized(self):
        renderer = self.viewport.renderer
        for panel in (self.viewport_panel, self.outliner, self.details, self.settings):
            panel.set_renderer(renderer)

        self._refresh_timer.start(PANEL_REFRESH_MS)
        self.statusBar().showMessage(HINT)

    def _on_failed(self, message):
        self.statusBar().showMessage("The renderer could not start")
        QMessageBox.critical(self, "DXI", f"The renderer could not start:\n\n{message}")

    def _refresh_panels(self):
        for panel in (self.viewport_panel, self.outliner, self.details, self.settings):
            panel.refresh()

    def _deselect(self):
        if self.viewport.has_engine:
            self.viewport.renderer.selected = -1

    def _about(self):
        QMessageBox.about(self, "DXI Editor",
                          f"DXI, a DirectX 12 deferred renderer.\n\nEngine module dxi {self._dxi.__version__}, "
                          "running in this process.")

    def closeEvent(self, event):
        # The GPU has to be done with the window's swap chain before the window is destroyed
        self._refresh_timer.stop()
        self.viewport.shutdown()
        super().closeEvent(event)
