"""The Options window: one tab for each entry of the Show menu, with everything that entry can be set to.

The Show menu only switches things on and off. How they look is here, in a window of its own that stays open
beside the editor, so a value can be changed while the scene is in view. It is not modal for that reason, and it
opens on the tab it was left on.

Every tab starts with the same switch as the Show menu, and the two stay in step: both read and write the
engine's setting, and the engine is the only copy.
"""

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QColorDialog, QDialog, QFormLayout, QPushButton, QTabWidget, QVBoxLayout, QWidget,
)

from widgets import Binder

# Tangent, bitangent and normal in the colours the engine draws them in
VECTOR_COLORS = {"show_tangent": "#ff5a5a", "show_bitangent": "#5aff5a", "show_normal": "#7399ff"}


class OptionsDialog(QDialog):
    def __init__(self, dxi, parent=None):
        super().__init__(parent)
        self._dxi = dxi
        self._renderer = None
        self._binder = Binder()

        self.setWindowTitle("Options")
        self.setWindowFlag(Qt.Tool)  # stays above the editor and does not take a taskbar button
        self.setModal(False)
        self.resize(500, 380)  # wide enough for all four tabs without scroll arrows

        self._tabs = QTabWidget()
        self._tabs.setDocumentMode(True)  # a bar of tabs, not a framed box around the page
        self._tabs.addTab(self._selection_page(), "Selection Highlight")
        self._tabs.addTab(self._gizmo_page(), "Transform Gizmo")
        self._tabs.addTab(self._tangent_page(), "Tangent Frame")
        self._tabs.addTab(self._axes_page(), "Origin Axes")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._tabs)

    # Pages

    @staticmethod
    def _page():
        page = QWidget()
        page.setObjectName("Panel")
        form = QFormLayout(page)
        form.setContentsMargins(14, 14, 14, 14)
        form.setHorizontalSpacing(14)
        form.setVerticalSpacing(8)
        form.setLabelAlignment(Qt.AlignLeft | Qt.AlignVCenter)
        form.setFieldGrowthPolicy(QFormLayout.ExpandingFieldsGrow)
        return page, form

    def _selection_page(self):
        b = self._binder
        selection = lambda r: r.selection

        self._color_button = QPushButton()
        self._color_button.clicked.connect(self._pick_color)

        page, form = self._page()
        form.addRow(b.check("Show", selection, "enabled"))
        form.addRow(b.check("Tint", selection, "show_tint"), b.spin(selection, "tint_strength", 0.0, 1.0, 0.05, 2))
        form.addRow(b.check("Box", selection, "show_box"), b.spin(selection, "box_thickness", 1.0, 8.0, 0.5, 1))
        form.addRow("Color", self._color_button)
        return page

    def _gizmo_page(self):
        b, dxi = self._binder, self._dxi
        gizmo = lambda r: r.gizmo

        page, form = self._page()
        form.addRow(b.check("Show", gizmo, "enabled"))
        form.addRow("Tool", b.combo(gizmo, "mode", [("Move  (1)", dxi.TransformMode.Move),
                                                     ("Rotate  (2)", dxi.TransformMode.Rotate)]))
        form.addRow("Space", b.combo(gizmo, "space", [("World", dxi.TransformSpace.World),
                                                       ("Local", dxi.TransformSpace.Local)]))
        form.addRow("Size", b.spin(gizmo, "size", 0.04, 0.40, 0.01, 2))
        form.addRow("Move snap (Ctrl)", b.spin(gizmo, "move_snap", 0.01, 100.0, 0.05, 2))
        form.addRow("Rotate snap (Ctrl)", b.spin(gizmo, "rotate_snap", 0.5, 180.0, 5.0, 1))
        return page

    def _tangent_page(self):
        b = self._binder
        tangent = lambda r: r.tangent_frame

        page, form = self._page()
        form.addRow(b.check("Show", tangent, "enabled"))

        for text, attr in (("Tangent (U)", "show_tangent"), ("Bitangent (V)", "show_bitangent"), ("Normal", "show_normal")):
            box = b.check(text, tangent, attr)
            box.setStyleSheet(f"color: {VECTOR_COLORS[attr]};")
            form.addRow(box)

        form.addRow(b.check("Depth test", tangent, "depth_test"))

        density = b.spin(tangent, "vertex_percent", 0.1, 100.0, 1.0, 1)
        density.setToolTip("Share of a dense mesh's vertices that draw their vectors. "
                           "Cubes, planes and small meshes always draw all of them.")
        form.addRow("Density (% of vertices)", density)
        form.addRow("Length", b.spin(tangent, "vector_length", 0.01, 5.0, 0.05, 2))
        form.addRow("Thickness (px)", b.spin(tangent, "line_thickness", 1.0, 12.0, 0.5, 1))
        return page

    def _axes_page(self):
        b = self._binder
        axes = lambda r: r.axes

        page, form = self._page()
        form.addRow(b.check("Show", axes, "enabled"))
        form.addRow(b.check("Depth test", axes, "depth_test"))
        form.addRow("Length", b.spin(axes, "axis_length", 0.05, 10.0, 0.05, 2))
        form.addRow("Thickness (px)", b.spin(axes, "line_thickness", 1.0, 12.0, 0.5, 1))
        form.addRow("Arrow head size (px)", b.spin(axes, "head_size", 4.0, 48.0, 1.0, 1))
        return page

    # Engine

    def set_renderer(self, renderer):
        self._renderer = renderer
        self._binder.bind(renderer)
        self._show_color()

    def refresh(self):
        # Nothing to keep up to date while it is closed
        if self.isVisible():
            self._binder.refresh()
            self._show_color()

    def open(self):
        """Shows the window, or brings it forward if it is already up"""
        self._binder.refresh()
        self._show_color()
        self.show()
        self.raise_()
        self.activateWindow()

    # Selection colour

    def _show_color(self):
        if self._renderer is None or not self._renderer.alive:
            return

        color = QColor.fromRgbF(*self._renderer.selection.color)
        self._color_button.setStyleSheet(f"background-color: {color.name()}; color: #000; border: 1px solid #000;")
        self._color_button.setText(color.name())

    def _pick_color(self):
        current = QColor.fromRgbF(*self._renderer.selection.color)
        color = QColorDialog.getColor(current, self, "Selection color")
        if color.isValid():
            self._renderer.selection.color = (color.redF(), color.greenF(), color.blueF())
            self._show_color()
