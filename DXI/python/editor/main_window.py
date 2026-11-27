"""The editor window, laid out as Unreal's is: a menu bar over the viewport, the outliner above the details on the
right, the content browser under the viewport, and a status bar below.

The middle holds tabs. The first is the viewport and stays. Opening an object (a double click on it in the view or
the outliner, or the Open in Tab button of the details) adds a tab named after it, with a preview to turn around,
its material and textures, see object_tab.py. Opening it again goes to the tab it already has.
"""

from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QAction
from PySide6.QtWidgets import QDockWidget, QMainWindow, QMessageBox, QScrollArea, QTabBar, QTabWidget

from content_browser import ContentBrowser
from object_tab import ObjectEditor
from outliner import OutlinerPanel, shifted
from panels import DetailsPanel
from viewport_panel import ViewportPanel

PANEL_REFRESH_MS = 100  # the engine changes things on its own, see panels.py
RIGHT_COLUMN_WIDTH = 400
BROWSER_HEIGHT = 250

HINT = ("Left click selects, drag a handle to move or rotate it (Ctrl snaps), double click opens the object in a tab.  "
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

        self._tabs = QTabWidget()
        self._tabs.setDocumentMode(True)
        self._tabs.setTabsClosable(True)
        self._tabs.setMovable(True)
        self._tabs.addTab(self.viewport_panel, "Viewport")
        # The viewport is not something to close
        self._tabs.tabBar().setTabButton(0, QTabBar.RightSide, None)
        self._tabs.tabBar().setTabButton(0, QTabBar.LeftSide, None)
        self._tabs.tabCloseRequested.connect(self._close_tab)
        self.setCentralWidget(self._tabs)
        self._editors = {}  # object index -> its tab

        self.outliner = OutlinerPanel()
        self.details = DetailsPanel(dxi)

        self.content_browser = ContentBrowser(dxi)

        outliner_dock = self._add_dock("Outliner", self.outliner)
        details_dock = self._add_dock("Details", self._scrolled(self.details))
        browser_dock = self._add_dock("Content Browser", self.content_browser)

        # Outliner on top, details under it
        self.addDockWidget(Qt.RightDockWidgetArea, outliner_dock)
        self.splitDockWidget(outliner_dock, details_dock, Qt.Vertical)
        self.resizeDocks([outliner_dock, details_dock], [320, 540], Qt.Vertical)
        self.resizeDocks([outliner_dock], [RIGHT_COLUMN_WIDTH], Qt.Horizontal)

        # The browser under the viewport only, the right column keeps the full height
        self.setCorner(Qt.BottomRightCorner, Qt.RightDockWidgetArea)
        self.addDockWidget(Qt.BottomDockWidgetArea, browser_dock)
        self.resizeDocks([browser_dock], [BROWSER_HEIGHT], Qt.Vertical)

        self._build_menus([outliner_dock, details_dock, browser_dock])
        self.statusBar().showMessage("Building the scene, the shaders are compiling...")

        self.viewport.initialized.connect(self._on_initialized)
        self.viewport.failed.connect(self._on_failed)
        self.viewport.selectionChanged.connect(self.outliner.show_selection)
        self.viewport.contentDropped.connect(self.content_browser.drop_rows)
        self.viewport.objectActivated.connect(self.open_object)
        self.outliner.openRequested.connect(self.open_object)
        self.outliner.deleteObjectsRequested.connect(self.delete_objects)
        self.viewport.deleteRequested.connect(self._delete_selected)
        self.details.openRequested.connect(self.open_object)

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
        for panel in self._panels():
            panel.set_renderer(renderer)

        self._refresh_timer.start(PANEL_REFRESH_MS)
        self.statusBar().showMessage(HINT)

    def _on_failed(self, message):
        self.statusBar().showMessage("The renderer could not start")
        QMessageBox.critical(self, "DXI", f"The renderer could not start:\n\n{message}")

    def _panels(self):
        return (self.viewport_panel, self.outliner, self.details, self.content_browser)

    def _refresh_panels(self):
        for panel in self._panels():
            panel.refresh()

        # Only the tab in front is looked at
        current = self._tabs.currentWidget()
        if isinstance(current, ObjectEditor):
            current.refresh()

    # Object tabs

    def open_object(self, index):
        """The tab of an object, made if it has none, and brought to the front"""
        if not self.viewport.has_engine or not 0 <= index < self.viewport.renderer.object_count:
            return

        renderer = self.viewport.renderer
        renderer.selected = index

        editor = self._editors.get(index)
        if editor is None:
            name = renderer.object_names()[index]
            editor = ObjectEditor(self._dxi, renderer, index, name)
            editor.contentDropped.connect(self.content_browser.assign_rows)
            self._editors[index] = editor
            self._tabs.addTab(editor, name)

        self._tabs.setCurrentWidget(editor)

    def _delete_selected(self):
        if self.viewport.has_engine and self.viewport.renderer.selected >= 0:
            self.delete_objects([self.viewport.renderer.selected])

    def delete_objects(self, indices):
        """Removes objects from the scene. The engine renumbers what is behind them, so every list here that
        holds an object's number follows: the tabs, the outliner's folders, the browser's pending addition."""
        if not self.viewport.has_engine:
            return

        renderer = self.viewport.renderer
        gone = {index for index in indices if 0 <= index < renderer.object_count}
        if not gone or renderer.delete_objects(sorted(gone)) != len(gone):
            return

        # An object that is gone has no tab, the others keep theirs under the number they now have
        for index, editor in list(self._editors.items()):
            if index in gone:
                self._close_tab(self._tabs.indexOf(editor))

        self._editors = {shifted(index, gone): editor for index, editor in self._editors.items()}
        for index, editor in self._editors.items():
            editor.index = index

        self.outliner.objects_deleted(gone)
        self.content_browser.objects_deleted(len(gone))
        self.statusBar().showMessage(f"Deleted {len(gone)} object" + ("" if len(gone) == 1 else "s"), 4000)

    def _close_tab(self, position):
        editor = self._tabs.widget(position)
        if not isinstance(editor, ObjectEditor):
            return

        self._tabs.removeTab(position)
        self._editors.pop(editor.index, None)
        editor.deleteLater()

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
