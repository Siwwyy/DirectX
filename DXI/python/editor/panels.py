"""The dock beside the viewport that shows the details of the selected object.

It reads the engine and writes to it directly, and keeps no copy of anything that matters. What is on
screen is refreshed from the engine on a timer, because the engine changes things without telling anyone: a
click in the view selects, a drag on a gizmo moves an object. A refresh never overwrites a field the user is
typing in.

What the viewport draws on top of the scene, and how, is not here: it belongs to the view and not to an object.
It is the Show menu and the Options window of the viewport's header, see viewport_panel.py.
"""

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout, QWidget

from widgets import Section, is_editing, make_spin, make_vec3_row, set_quietly


class DetailsPanel(QWidget):
    """What the selected object is: where it stands, how it is turned, how big it is"""

    openRequested = Signal(int)  # the button under the title: the selected object's own tab

    def __init__(self, dxi, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._dxi = dxi
        self._renderer = None
        self._shown_index = -2  # which object the fields were last filled for

        self._title = QLabel("")
        self._title.setObjectName("DetailsTitle")

        self._open = QPushButton("Open in Tab")
        self._open.setToolTip("The object's own tab: a preview to turn around, its material and textures. Double click an object to open it too.")
        self._open.clicked.connect(lambda: self.openRequested.emit(self._renderer.selected))

        self._open_row = QWidget()
        open_layout = QHBoxLayout(self._open_row)
        open_layout.setContentsMargins(10, 0, 10, 8)
        open_layout.addWidget(self._open)
        open_layout.addStretch(1)

        self._position = [make_spin(-1e6, 1e6, 0.1) for _ in range(3)]
        self._rotation = [make_spin(-36000, 36000, 1.0, 2) for _ in range(3)]
        self._scale = [make_spin(-1e4, 1e4, 0.05) for _ in range(3)]
        for box in self._position + self._rotation + self._scale:
            box.valueChanged.connect(self._write_transform)

        self._transform = Section("Transform")
        self._transform.add_row("Location", make_vec3_row(self._position))
        self._transform.add_row("Rotation", make_vec3_row(self._rotation))
        self._transform.add_row("Scale", make_vec3_row(self._scale))

        self._empty = QLabel("Select an object in the viewport or the outliner to see its details.")
        self._empty.setObjectName("Dim")
        self._empty.setWordWrap(True)
        self._empty.setMargin(10)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._title)
        layout.addWidget(self._open_row)
        layout.addWidget(self._empty)
        layout.addWidget(self._transform)
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
        self._open_row.setVisible(has_object)
        self._transform.setVisible(has_object)
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

        self._shown_index = index

    def _focused_box(self):
        for box in self._position + self._rotation + self._scale:
            if is_editing(box):
                return box
        return None

    def _write_transform(self):
        renderer = self._renderer
        index = renderer.selected if renderer is not None and renderer.alive else -1
        if index >= 0:
            renderer.set_transform(index, self._dxi.Transform(
                position=tuple(box.value() for box in self._position),
                rotation=tuple(box.value() for box in self._rotation),
                scale=tuple(box.value() for box in self._scale)))
