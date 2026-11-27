"""The editor tab of one object of the scene, like the asset editors of Unreal.

A preview of the mesh that can be orbited, or with Rotate Model on, turned. Beside it the preview's own transform,
the material the object wears with a picker, and the textures of that material in their slots. Double click a
texture to see it large. A material tile dragged from the content browser onto the preview is put on the object.

The transform here is not the object's. It belongs to the preview, starts as the mesh was made and changes nothing in
the scene, the way the asset editors of Unreal keep their own view of an asset. From Scene and To Scene copy it
across when that is wanted. The material is the object's: it is read back from the engine on the panels' timer, so a
change made elsewhere shows here, and the other way round.

The engine reports which material an object wears by name, the files of its textures come from the catalog of
materials found on disk. A material the scene made itself, which is not in the catalog, shows its slots without
pictures.
"""

import os
import time

from PySide6.QtCore import QSize, Qt, Signal
from PySide6.QtGui import QColor, QFontMetrics, QPainter, QPixmap
from PySide6.QtWidgets import (
    QComboBox, QDialog, QFrame, QGridLayout, QHBoxLayout, QLabel, QPushButton, QScrollArea, QSplitter, QToolButton,
    QVBoxLayout, QWidget,
)

import images
from mesh_preview import MODES, MeshData, MeshPreview
from theme import BORDER, FIELD, TEXT_DIM
from widgets import Section, make_spin, make_vec3_row, set_quietly

# The engine's six material slots, in its order, and the file each is read from: the name of the material and this
SLOTS = (
    ("Diffuse",           "_diff_1k.png"),
    ("Normal",            "_nor_dx_1k.png"),
    ("Specular",          "_spec_1k.png"),
    ("Metalness",         "_metal_1k.png"),
    ("Roughness",         "_rough_1k.png"),
    ("Ambient Occlusion", "_ao_1k.png"),
)

DEFAULT_MATERIAL = "Default Material"
TILE_SIDE = 100
COLUMNS = 3
ASSIGN_WAIT_S = 1.5  # how long the engine gets to take up a material before the picker goes back to what it wears


def material_files(row):
    """The six texture files of a catalog material, a path or None for each slot. row is what
    catalog_materials returns: (name, folder, lone colour map, loaded index)."""
    name, folder, diffuse_only, _ = row
    if folder:
        paths = [os.path.join(folder, name + suffix) for _, suffix in SLOTS]
        return [path if os.path.isfile(path) else None for path in paths]
    if diffuse_only:
        return [diffuse_only] + [None] * (len(SLOTS) - 1)
    return [None] * len(SLOTS)


def _placeholder(side, text):
    pixmap = QPixmap(side, side)
    pixmap.fill(QColor(FIELD))
    painter = QPainter(pixmap)
    painter.setPen(QColor(TEXT_DIM))
    painter.drawText(pixmap.rect(), Qt.AlignCenter | Qt.TextWordWrap, text)
    painter.end()
    return pixmap


class ImageViewer(QDialog):
    """A texture at a size that can be looked at, shrunk to the screen when it is larger"""

    def __init__(self, title, path, parent=None):
        super().__init__(parent)
        self.setWindowTitle(title)

        pixmap = QPixmap(path)
        screen = self.screen().availableGeometry() if self.screen() else None
        if screen is not None and not pixmap.isNull():
            limit = QSize(int(screen.width() * 0.8), int(screen.height() * 0.8))
            if pixmap.width() > limit.width() or pixmap.height() > limit.height():
                pixmap = pixmap.scaled(limit, Qt.KeepAspectRatio, Qt.SmoothTransformation)

        label = QLabel()
        label.setPixmap(pixmap)
        label.setAlignment(Qt.AlignCenter)
        label.setToolTip(path)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(label)


class TextureTile(QFrame):
    """One slot of a material: its picture, the name of the slot and what file is in it"""

    activated = Signal(str, str)  # slot name, file path

    def __init__(self, slot, path, has_value, parent=None):
        super().__init__(parent)
        self._slot, self._path = slot, path
        self.setFixedWidth(TILE_SIDE + 8)

        picture = QLabel()
        picture.setFixedSize(TILE_SIDE, TILE_SIDE)
        picture.setStyleSheet(f"border: 1px solid {BORDER};")

        size = images.image_size(path) if path else None
        image = images.load_image(path) if size else None
        if image is not None:
            picture.setPixmap(QPixmap.fromImage(image).scaled(TILE_SIDE, TILE_SIDE, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        else:
            picture.setPixmap(_placeholder(TILE_SIDE - 2, "Solid value" if has_value else "Not set"))
        picture.setAlignment(Qt.AlignCenter)

        title = QLabel(slot)
        title.setAlignment(Qt.AlignHCenter)

        detail = QLabel()
        detail.setObjectName("Dim")
        detail.setAlignment(Qt.AlignHCenter)
        if size:
            elided = QFontMetrics(detail.font()).elidedText(os.path.basename(path), Qt.ElideMiddle, TILE_SIDE + 4)
            detail.setText(f"{size[0]} \u00d7 {size[1]}\n{elided}")
            self.setToolTip(f"{slot}\n{path}\nDouble click to open")
            self.setCursor(Qt.PointingHandCursor)
        else:
            detail.setText("\n")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        layout.addWidget(picture)
        layout.addWidget(title)
        layout.addWidget(detail)

    def mouseDoubleClickEvent(self, event):
        if self._path and os.path.isfile(self._path):
            self.activated.emit(self._slot, self._path)


class ObjectEditor(QWidget):
    contentDropped = Signal(int, list)  # this object's index and the rows of the tiles let go on its preview

    def __init__(self, dxi, renderer, index, name, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._dxi = dxi
        self._renderer = renderer
        self.index = index
        self.name = name

        self._catalog = renderer.catalog_materials()
        self._state = None            # (material name, has its own) the picker and the tiles were last filled for
        self._combo_catalog = []      # the catalog row each picker entry stands for, -1 for the default, None for none
        self._expect_until = 0.0

        # The mesh, once: it does not change for as long as the object exists
        self._mesh = MeshData.from_engine(renderer, index)

        self._preview = MeshPreview()
        self._preview.set_mesh(self._mesh)
        self._preview.objectRotated.connect(self._on_object_rotated)
        self._preview.contentDropped.connect(lambda rows: self.contentDropped.emit(self.index, rows))

        # The same widgets the viewport's header is made of, so they look the same
        header = QFrame()
        header.setObjectName("ViewportHeader")
        row = QHBoxLayout(header)
        row.setContentsMargins(6, 3, 6, 3)

        self._mode = QComboBox()
        self._mode.addItems(MODES)
        self._mode.setToolTip("How the mesh is drawn")
        self._mode.currentTextChanged.connect(self._preview.set_mode)

        self._rotate = QToolButton()
        self._rotate.setText("Rotate Model")
        self._rotate.setCheckable(True)
        self._rotate.setToolTip("Dragging turns the model in this preview instead of the camera. The object in the scene stays as it is.")
        self._rotate.toggled.connect(self._preview.set_rotate_object)

        frame = QToolButton()
        frame.setText("Frame")
        frame.setToolTip("Back to a view of the whole object (F)")
        frame.clicked.connect(self._preview.frame)

        row.addWidget(self._mode)
        row.addWidget(self._rotate)
        row.addWidget(frame)
        row.addStretch(1)

        left = QWidget()
        column = QVBoxLayout(left)
        column.setContentsMargins(0, 0, 0, 0)
        column.setSpacing(0)
        column.addWidget(header)
        column.addWidget(self._preview, 1)

        # The preview's own pose. It starts as the mesh was made (no rotation, scale 1) and is not the object's
        # transform in the scene, which the Details panel edits, so turning it here moves nothing in the viewport.
        # There is no location, the preview always centres the mesh.
        self._rotation = [make_spin(-36000, 36000, 1.0, 2) for _ in range(3)]
        self._scale = [make_spin(-1e4, 1e4, 0.05) for _ in range(3)]
        for box in self._scale:
            box.setValue(1.0)
        for box in self._rotation + self._scale:
            box.valueChanged.connect(self._on_pose_edited)

        reset = QPushButton("Reset")
        reset.setToolTip("The mesh as it was made: no rotation, scale 1")
        reset.clicked.connect(lambda: self._set_pose((0.0, 0.0, 0.0), (1.0, 1.0, 1.0)))
        from_scene = QPushButton("From Scene")
        from_scene.setToolTip("Take the rotation and scale the object has in the scene")
        from_scene.clicked.connect(self._pose_from_scene)
        to_scene = QPushButton("To Scene")
        to_scene.setToolTip("Give the object in the scene this rotation and scale")
        to_scene.clicked.connect(self._pose_to_scene)

        buttons = QWidget()
        button_row = QHBoxLayout(buttons)
        button_row.setContentsMargins(0, 0, 0, 0)
        button_row.addWidget(reset)
        button_row.addWidget(from_scene)
        button_row.addWidget(to_scene)
        button_row.addStretch(1)

        transform = Section("Preview Transform")
        transform.setToolTip("Only for this preview. The object's transform in the scene is in the Details panel.")
        transform.add_row("Rotation", make_vec3_row(self._rotation))
        transform.add_row("Scale", make_vec3_row(self._scale))
        transform.add_row(buttons)

        # Material
        self._picker = QComboBox()
        self._picker.setToolTip("The material the object is drawn with. Materials are found on disk, see the Materials category of the content browser.")
        self._picker.activated.connect(self._on_picked)

        self._grid_host = QWidget()
        self._grid = QGridLayout(self._grid_host)
        self._grid.setContentsMargins(0, 0, 0, 0)
        self._grid.setHorizontalSpacing(6)
        self._grid.setVerticalSpacing(8)
        self._grid.setAlignment(Qt.AlignLeft | Qt.AlignTop)

        material = Section("Material")
        material.add_row("Material", self._picker)
        material.add_row(self._grid_host)

        # Mesh
        mesh = Section("Mesh", expanded=False)
        mesh.add_row("Vertices", QLabel(f"{len(self._mesh.positions):,}"))
        mesh.add_row("Triangles", QLabel(f"{len(self._mesh.triangles):,}"))
        mesh.add_row("UVs", QLabel("yes" if self._mesh.uvs is not None else "no"))
        mesh.add_row("Normals", QLabel("yes" if self._mesh.normals is not None else "computed per face"))

        title = QLabel(name)
        title.setObjectName("DetailsTitle")

        side = QWidget()
        stack = QVBoxLayout(side)
        stack.setContentsMargins(0, 0, 0, 0)
        stack.setSpacing(0)
        stack.addWidget(title)
        stack.addWidget(transform)
        stack.addWidget(material)
        stack.addWidget(mesh)
        stack.addStretch(1)

        scroll = QScrollArea()
        scroll.setWidget(side)
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        scroll.setMinimumWidth(3 * (TILE_SIDE + 8) + 2 * 6 + 44)

        split = QSplitter(Qt.Horizontal)
        split.addWidget(left)
        split.addWidget(scroll)
        split.setStretchFactor(0, 1)
        split.setCollapsible(0, False)
        split.setSizes([900, 400])

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(split)

        self.refresh()

    # Engine

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive or self.index >= renderer.object_count:
            return

        # Only the material is read back. The pose is this tab's own, nothing in the scene moves it.
        self._sync_material()

    # The preview's pose

    def _pose(self):
        return (tuple(box.value() for box in self._rotation), tuple(box.value() for box in self._scale))

    def _on_pose_edited(self):
        self._preview.set_transform(*self._pose())

    def _set_pose(self, rotation, scale):
        for box, value in zip(self._rotation + self._scale, tuple(rotation) + tuple(scale)):
            set_quietly(box, box.setValue, value)
        self._on_pose_edited()

    def _on_object_rotated(self, x, y, z):
        # The preview turned the model by a drag, the fields follow
        for box, value in zip(self._rotation, (x, y, z)):
            set_quietly(box, box.setValue, value)

    def _pose_from_scene(self):
        if self._renderer is not None and self._renderer.alive:
            transform = self._renderer.get_transform(self.index)
            self._set_pose(transform.rotation, transform.scale)

    def _pose_to_scene(self):
        if self._renderer is not None and self._renderer.alive:
            rotation, scale = self._pose()
            # The location stays where the object is
            position = self._renderer.get_transform(self.index).position
            self._renderer.set_transform(self.index, self._dxi.Transform(
                position=tuple(position), rotation=rotation, scale=scale))

    # Material

    def _catalog_row_of(self, name):
        for row, entry in enumerate(self._catalog):
            if entry[0] == name:
                return row
        return None

    def _sync_material(self):
        name, has_own, _two_sided, has_texture = self._renderer.object_material(self.index)
        state = (name, has_own)

        # The engine takes a request at the start of a frame, so for a moment it still reports the old material and
        # the picker already shows the new one. When it never changes, the engine refused (the material heap is
        # full), and the picker goes back to what the object really wears.
        if self._expect_until and time.monotonic() > self._expect_until:
            self._expect_until = 0.0
            self._state = None

        if state == self._state:
            return

        self._state = state
        self._expect_until = 0.0

        row = self._catalog_row_of(name) if has_own else None

        # The picker: the default, every material on disk, and the object's own when the scene made it
        labels, rows = [DEFAULT_MATERIAL], [-1]
        for number, entry in enumerate(self._catalog):
            labels.append(entry[0])
            rows.append(number)
        if has_own and row is None:
            labels.insert(1, f"{name}  (scene)")
            rows.insert(1, None)

        self._combo_catalog = rows
        with_current = rows.index(row) if row is not None else (1 if has_own else 0)
        self._picker.blockSignals(True)
        self._picker.clear()
        self._picker.addItems(labels)
        self._picker.setCurrentIndex(with_current)
        self._picker.blockSignals(False)

        files = material_files(self._catalog[row]) if row is not None else [None] * len(SLOTS)
        self._fill_tiles(files, has_texture)
        self._preview.set_texture(images.load_rgb(files[0], 512) if files[0] else None)

    def _fill_tiles(self, files, has_texture):
        while self._grid.count():
            item = self._grid.takeAt(0)
            if item.widget() is not None:
                item.widget().deleteLater()

        for number, ((slot, _), path) in enumerate(zip(SLOTS, files)):
            tile = TextureTile(slot, path, bool(has_texture[number]))
            tile.activated.connect(self._open_texture)
            self._grid.addWidget(tile, number // COLUMNS, number % COLUMNS)

    def _open_texture(self, slot, path):
        ImageViewer(f"{self.name}  |  {slot}", path, self.window()).exec()

    def _on_picked(self, position):
        row = self._combo_catalog[position]
        if row is None or self._renderer is None or not self._renderer.alive:
            return
        self._renderer.assign_material(self.index, row)
        self._expect_until = time.monotonic() + ASSIGN_WAIT_S
