"""The docks around the viewport: the outliner, the details of the selected object, and the viewport settings.

All three read the engine and write to it directly, and none keeps a copy of anything that matters. What is on
screen is refreshed from the engine on a timer, because the engine changes things without telling anyone: a
click in the view selects, a drag on a gizmo moves an object, the 1 and 2 keys switch the tool. A refresh never
overwrites a field the user is typing in.
"""

from PySide6.QtCore import QSignalBlocker
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QCheckBox, QColorDialog, QHBoxLayout, QLabel, QLineEdit, QListWidget, QPushButton, QVBoxLayout, QWidget,
)

from widgets import Binder, Section, is_editing, make_spin, make_vec3_row, set_quietly


class OutlinerPanel(QWidget):
    """Every object of the scene, in the order the engine holds them, which is what its indices are"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._renderer = None
        self._names = []

        self._search = QLineEdit()
        self._search.setPlaceholderText("Search...")
        self._search.setClearButtonEnabled(True)
        self._search.textChanged.connect(self._apply_filter)

        self._list = QListWidget()
        self._list.setAlternatingRowColors(True)
        self._list.currentRowChanged.connect(self._on_row_changed)

        self._count = QLabel("")
        self._count.setObjectName("Dim")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 6, 6, 4)
        layout.setSpacing(6)
        layout.addWidget(self._search)
        layout.addWidget(self._list, 1)
        layout.addWidget(self._count)

    def set_renderer(self, renderer):
        self._renderer = renderer
        self.refresh()

    def refresh(self):
        if self._renderer is None or not self._renderer.alive:
            return

        names = self._renderer.object_names()
        if names != self._names:
            self._names = names
            with QSignalBlocker(self._list):
                self._list.clear()
                self._list.addItems(names)
            self._apply_filter()

        self.show_selection(self._renderer.selected)

    def show_selection(self, index):
        if self._list.currentRow() != index:
            with QSignalBlocker(self._list):
                self._list.setCurrentRow(index)
                if index < 0:
                    self._list.clearSelection()

        selected = "" if index < 0 else " (1 selected)"
        self._count.setText(f"{len(self._names)} objects{selected}")

    def _apply_filter(self):
        # Hidden, not removed: a row keeps its number, and the number is the engine's index
        needle = self._search.text().strip().lower()
        for row, name in enumerate(self._names):
            self._list.item(row).setHidden(bool(needle) and needle not in name.lower())

    def _on_row_changed(self, row):
        if self._renderer is not None and self._renderer.alive:
            self._renderer.selected = row


class DetailsPanel(QWidget):
    """What the selected object is: where it stands, and which overlays it takes part in"""

    def __init__(self, dxi, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._dxi = dxi
        self._renderer = None
        self._shown_index = -2  # which object the fields were last filled for

        self._title = QLabel("")
        self._title.setObjectName("DetailsTitle")

        self._position = [make_spin(-1e6, 1e6, 0.1) for _ in range(3)]
        self._rotation = [make_spin(-36000, 36000, 1.0, 2) for _ in range(3)]
        self._scale = [make_spin(-1e4, 1e4, 0.05) for _ in range(3)]
        for box in self._position + self._rotation + self._scale:
            box.valueChanged.connect(self._write_transform)

        self._transform = Section("Transform")
        self._transform.add_row("Location", make_vec3_row(self._position))
        self._transform.add_row("Rotation", make_vec3_row(self._rotation))
        self._transform.add_row("Scale", make_vec3_row(self._scale))

        # Each only draws once its overlay is switched on, in Viewport Settings or the Show menu
        self._tangent_frame = QCheckBox("Tangent frame")
        self._origin_axes = QCheckBox("Origin axes")
        self._tangent_frame.toggled.connect(self._write_flags)
        self._origin_axes.toggled.connect(self._write_flags)

        self._rendering = Section("Rendering")
        self._rendering.add_row("Show", self._tangent_frame)
        self._rendering.add_row("", self._origin_axes)

        self._empty = QLabel("Select an object in the viewport or the outliner to see its details.")
        self._empty.setObjectName("Dim")
        self._empty.setWordWrap(True)
        self._empty.setMargin(10)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._title)
        layout.addWidget(self._empty)
        layout.addWidget(self._transform)
        layout.addWidget(self._rendering)
        layout.addStretch(1)

    def set_renderer(self, renderer):
        self._renderer = renderer
        self._shown_index = -2
        self.refresh()

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        index = renderer.selected
        has_object = index >= 0

        self._title.setVisible(has_object)
        self._transform.setVisible(has_object)
        self._rendering.setVisible(has_object)
        self._empty.setVisible(not has_object)
        if not has_object:
            self._shown_index = index
            return

        names = renderer.object_names()
        self._title.setText(names[index] if index < len(names) else "")

        # A field being typed in keeps what the user has there. A different object means everything
        # is replaced regardless, the typing was about the one before.
        typing = None if index != self._shown_index else self._focused_box()
        transform = renderer.get_transform(index)

        for boxes, values in ((self._position, transform.position),
                              (self._rotation, transform.rotation),
                              (self._scale, transform.scale)):
            for box, value in zip(boxes, values):
                if box is not typing:
                    set_quietly(box, box.setValue, value)

        tangent_frame, origin_axes = renderer.get_overlay_flags(index)
        set_quietly(self._tangent_frame, self._tangent_frame.setChecked, tangent_frame)
        set_quietly(self._origin_axes, self._origin_axes.setChecked, origin_axes)
        self._shown_index = index

    def _focused_box(self):
        for box in self._position + self._rotation + self._scale:
            if is_editing(box):
                return box
        return None

    def _selected_index(self):
        renderer = self._renderer
        return renderer.selected if renderer is not None and renderer.alive else -1

    def _write_transform(self):
        index = self._selected_index()
        if index >= 0:
            self._renderer.set_transform(index, self._dxi.Transform(
                position=tuple(box.value() for box in self._position),
                rotation=tuple(box.value() for box in self._rotation),
                scale=tuple(box.value() for box in self._scale)))

    def _write_flags(self):
        index = self._selected_index()
        if index >= 0:
            self._renderer.set_overlay_flags(index,
                                             tangent_frame=self._tangent_frame.isChecked(),
                                             origin_axes=self._origin_axes.isChecked())


class SettingsPanel(QWidget):
    """The settings of the viewport's overlays, which belong to the view and not to an object"""

    def __init__(self, dxi, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._dxi = dxi
        self._renderer = None
        self._binder = Binder()
        self._flag_labels = []  # (label, overlay settings, which flag), kept up to date by refresh

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._build_gizmo())
        layout.addWidget(self._build_selection())
        layout.addWidget(self._build_tangent_frame())
        layout.addWidget(self._build_axes())
        layout.addStretch(1)

    def set_renderer(self, renderer):
        self._renderer = renderer
        self._binder.bind(renderer)
        self.refresh()

    def refresh(self):
        self._binder.refresh()
        self._show_color(self._renderer.selection.color if self._renderer and self._renderer.alive else None)
        self._refresh_flag_counts()

    # Sections

    def _build_gizmo(self):
        b, dxi = self._binder, self._dxi
        gizmo = lambda r: r.gizmo

        section = Section("Transform Gizmo")
        section.add_row(b.check("Enabled", gizmo, "enabled"))
        section.add_row("Tool", b.combo(gizmo, "mode", [("Move  (1)", dxi.TransformMode.Move),
                                                         ("Rotate  (2)", dxi.TransformMode.Rotate)]))
        section.add_row("Space", b.combo(gizmo, "space", [("World", dxi.TransformSpace.World),
                                                           ("Local", dxi.TransformSpace.Local)]))
        section.add_row("Size", b.spin(gizmo, "size", 0.04, 0.40, 0.01, 2))
        section.add_row("Move snap (Ctrl)", b.spin(gizmo, "move_snap", 0.01, 100.0, 0.05, 2))
        section.add_row("Rotate snap (Ctrl)", b.spin(gizmo, "rotate_snap", 0.5, 180.0, 5.0, 1))
        return section

    def _build_selection(self):
        b = self._binder
        selection = lambda r: r.selection

        self._color_button = QPushButton()
        self._color_button.clicked.connect(self._pick_color)

        section = Section("Selection Highlight")
        section.add_row(b.check("Enabled", selection, "enabled"))
        section.add_row(b.check("Tint", selection, "show_tint"), b.spin(selection, "tint_strength", 0.0, 1.0, 0.05, 2))
        section.add_row(b.check("Box", selection, "show_box"), b.spin(selection, "box_thickness", 1.0, 8.0, 0.5, 1))
        section.add_row("Color", self._color_button)
        return section

    def _build_tangent_frame(self):
        b = self._binder
        tangent = lambda r: r.tangent_frame

        section = Section("Tangent Frame")
        section.add_row(b.check("Enabled", tangent, "enabled"))
        section.add_row(self._flag_row(tangent, 0, "Objects"))

        # One check box per vector, in the colour it is drawn in
        for text, attr, color in (("Tangent (U)", "show_tangent", "#ff5a5a"),
                                  ("Bitangent (V)", "show_bitangent", "#5aff5a"),
                                  ("Normal", "show_normal", "#7399ff")):
            box = b.check(text, tangent, attr)
            box.setStyleSheet(f"color: {color};")
            section.add_row(box)

        section.add_row(b.check("Depth test", tangent, "depth_test"))
        section.add_row("Vector length", b.spin(tangent, "vector_length", 0.01, 5.0, 0.05, 2))
        section.add_row("Line thickness", b.spin(tangent, "line_thickness", 1.0, 12.0, 0.5, 1))
        section.add_row("Vertices shown %", b.spin(tangent, "vertex_percent", 0.1, 100.0, 1.0, 1))
        return section

    def _build_axes(self):
        b = self._binder
        axes = lambda r: r.axes

        section = Section("Origin Axes")
        section.add_row(b.check("Enabled", axes, "enabled"))
        section.add_row(self._flag_row(axes, 1, "Objects"))
        section.add_row(b.check("Depth test", axes, "depth_test"))
        section.add_row("Axis length", b.spin(axes, "axis_length", 0.05, 10.0, 0.05, 2))
        section.add_row("Line thickness", b.spin(axes, "line_thickness", 1.0, 12.0, 0.5, 1))
        section.add_row("Arrow head size", b.spin(axes, "head_size", 4.0, 48.0, 1.0, 1))
        return section

    def _flag_row(self, resolve, which, label):
        """Which objects take part in an overlay is a flag on each of them. This is the whole scene's at once,
        and says how many there are, since an overlay that is on with nothing flagged draws nothing."""
        text = QLabel("")
        all_button = QPushButton("All")
        none_button = QPushButton("None")
        all_button.clicked.connect(lambda: self._flag_all(which, True))
        none_button.clicked.connect(lambda: self._flag_all(which, False))
        self._flag_labels.append((text, resolve, which))

        row = QWidget()
        layout = QHBoxLayout(row)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(text, 1)
        layout.addWidget(all_button)
        layout.addWidget(none_button)
        return row

    # Overlay flags

    def _flag_all(self, which, value):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        for index in range(renderer.object_count):
            if which == 0:
                renderer.set_overlay_flags(index, tangent_frame=value)
            else:
                renderer.set_overlay_flags(index, origin_axes=value)

    def _refresh_flag_counts(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        total = renderer.object_count
        flags = [renderer.get_overlay_flags(index) for index in range(total)]

        for label, resolve, which in self._flag_labels:
            flagged = sum(1 for pair in flags if pair[which])
            label.setText(f"{flagged} of {total} flagged")

            # On, and nothing to draw it on: the one state that looks like a bug
            idle = resolve(renderer).enabled and flagged == 0
            label.setObjectName("Warning" if idle else "Dim")
            label.setStyleSheet("")  # re-reads the style for the new name
            label.style().unpolish(label)
            label.style().polish(label)

    # Selection colour

    def _show_color(self, rgb):
        if rgb is None:
            return

        color = QColor.fromRgbF(*rgb)
        self._color_button.setStyleSheet(f"background-color: {color.name()}; color: #000; border: 1px solid #000;")
        self._color_button.setText(color.name())

    def _pick_color(self):
        current = QColor.fromRgbF(*self._renderer.selection.color)
        color = QColorDialog.getColor(current, self, "Selection color")
        if color.isValid():
            self._renderer.selection.color = (color.redF(), color.greenF(), color.blueF())
            self._show_color(self._renderer.selection.color)
