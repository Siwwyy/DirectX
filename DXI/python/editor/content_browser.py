"""The content browser: what the engine can add to the scene, as square thumbnails.

Engine content is the three built in shapes, the models and materials found on disk (the engine's catalog) and the four
kinds of light. A double click, Enter or the context menu adds the selected tiles in front of the camera, and a drag
from a tile onto the viewport adds them where it is dropped, standing on what is under the cursor. The new object is
selected so it can be moved at once.

A material is not added but put on an object: the selected one for a double click, the one under the cursor for a drop
(which also selects it), or the one of an open object tab it is dropped on.

The categories on show come from the tree (one at a time) or the Filters menu (any mix), which share one state, and
the search narrows what they leave.

An addition is a request the engine carries out at the start of its next frame, so it shows up in the scene a moment
after the click. The browser watches the object count for that, and selects the newest object when it appears.
"""

import os
import time
from dataclasses import dataclass
from typing import Callable

from PySide6.QtCore import QMimeData, QModelIndex, QRect, QSize, Qt, Signal
from PySide6.QtGui import QAction, QColor, QDrag, QFontMetrics, QIcon, QPainter, QPen, QPixmap
from PySide6.QtWidgets import (
    QAbstractItemView, QFrame, QHBoxLayout, QLabel, QLineEdit, QListView, QListWidget, QListWidgetItem, QMenu,
    QSlider, QSplitter, QStyle, QStyledItemDelegate, QToolButton, QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget,
)

import thumbnails
from theme import ACCENT, BORDER, HOVER, SELECTION, TEXT
from viewport import CONTENT_MIME
from widgets import set_quietly

ENTRY = Qt.UserRole

TILE_MIN, TILE_MAX, TILE_DEFAULT = 64, 200, 112  # the side of the square thumbnail, in pixels
PAD = 8        # around a tile
LABEL_HEIGHT = 20
STRIP = 3      # the coloured bar along the foot of a thumbnail that says what kind of asset it is
DROP_SPREAD_PX = 60  # between tiles dropped together, in pixels of the view
SPAWN_WAIT_S = 2.0  # how long to wait for the engine to carry a request out before giving up on it

# One colour for each kind, as Unreal gives each asset type its own
KIND_COLORS = {"Shapes": QColor("#4da3ff"), "Models": QColor("#3ec97a"), "Materials": QColor("#b57cff"), "Lights": QColor("#ffb43c")}


def _swatch(color):
    """A small square of a category's colour, for the menu and the tree"""
    pixmap = QPixmap(12, 12)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.Antialiasing)
    painter.setPen(Qt.NoPen)
    painter.setBrush(color)
    painter.drawRoundedRect(1, 1, 10, 10, 2, 2)
    painter.end()
    return QIcon(pixmap)


@dataclass
class Entry:
    name: str
    kind: str                         # "Shapes", "Models", "Materials" or "Lights"
    thumbnail: Callable               # makes the square pixmap, called when it is first drawn
    add: Callable                     # (renderer, at) -> None, files the request. at is a pixel of the view or None
    tooltip: str
    payload: int = -1                 # for a material, its number in the engine's catalog


class TileDelegate(QStyledItemDelegate):
    """Draws a tile as a square thumbnail with its name under it"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.side = TILE_DEFAULT

    def cell(self):
        return QSize(self.side + PAD * 2, self.side + LABEL_HEIGHT + PAD * 2)

    def sizeHint(self, option, index):
        return self.cell()

    def paint(self, painter, option, index):
        entry = index.data(ENTRY)
        if entry is None:
            return

        selected = bool(option.state & QStyle.State_Selected)
        hovered = bool(option.state & QStyle.State_MouseOver)
        cell = option.rect
        square = QRect(cell.left() + (cell.width() - self.side) // 2, cell.top() + PAD, self.side, self.side)

        painter.save()
        painter.setRenderHint(QPainter.SmoothPixmapTransform)

        if selected or hovered:
            painter.fillRect(cell.adjusted(2, 2, -2, -2), QColor(SELECTION) if selected else QColor(HOVER))

        painter.drawPixmap(square, entry.thumbnail())

        painter.fillRect(QRect(square.left(), square.bottom() - STRIP + 1, square.width(), STRIP), KIND_COLORS[entry.kind])

        # The frame sits inside the square, so the thumbnail keeps exactly the size the slider says
        painter.setBrush(Qt.NoBrush)
        painter.setPen(QPen(QColor(ACCENT) if selected else QColor("#5a5a5a") if hovered else QColor(BORDER), 2 if selected else 1))
        painter.drawRect(square.adjusted(0, 0, -1, -1))

        painter.setPen(QColor("#ffffff") if selected else QColor(TEXT))
        label = QRect(cell.left() + 4, square.bottom() + 4, cell.width() - 8, LABEL_HEIGHT)
        text = QFontMetrics(painter.font()).elidedText(entry.name, Qt.ElideRight, label.width())
        painter.drawText(label, Qt.AlignHCenter | Qt.AlignTop, text)

        painter.restore()


class TileList(QListWidget):
    """The grid of tiles. Enter adds the selection, which the list widget has no signal for, and a tile can be
    dragged out of it onto the viewport"""

    addRequested = Signal(list)

    def keyPressEvent(self, event):
        if event.key() in (Qt.Key_Return, Qt.Key_Enter) and self.selectedItems():
            self.addRequested.emit(self.selectedItems())
            return
        super().keyPressEvent(event)

    def drag_mime(self, items):
        """What a drag of these tiles carries: only the numbers of their rows, the viewport hands them back and
        the browser knows what they are"""
        mime = QMimeData()
        mime.setData(CONTENT_MIME, ",".join(str(self.row(item)) for item in items).encode())
        return mime

    def startDrag(self, supported_actions):
        items = self.selectedItems()
        if not items:
            return

        drag = QDrag(self)
        drag.setMimeData(self.drag_mime(items))

        # The thumbnail follows the cursor, held by its middle, and a stack of them says how many
        side = 80
        picture = QPixmap(side + 8 * (min(len(items), 3) - 1), side + 8 * (min(len(items), 3) - 1))
        picture.fill(Qt.transparent)
        painter = QPainter(picture)
        painter.setRenderHint(QPainter.SmoothPixmapTransform)
        painter.setOpacity(0.85)
        for stack, item in enumerate(reversed(items[:3])):
            offset = 8 * stack
            painter.drawPixmap(offset, offset, item.data(ENTRY).thumbnail().scaled(side, side, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        painter.end()
        drag.setPixmap(picture)
        drag.setHotSpot(picture.rect().center())

        drag.exec(Qt.CopyAction)


class ContentBrowser(QWidget):
    def __init__(self, dxi, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._dxi = dxi
        self._renderer = None
        self._kinds = set(KIND_COLORS)        # the categories on show, the tree and the Filters menu both set it
        self._object_target = None            # object count the engine is about to reach, None when idle
        self._light_target = None
        self._deadline = 0.0
        self._announce = ""                   # what was added, shown once it has arrived

        self._search = QLineEdit()
        self._search.setPlaceholderText("Search Engine")
        self._search.setClearButtonEnabled(True)
        self._search.textChanged.connect(self._apply_filter)

        self._size = QSlider(Qt.Horizontal)
        self._size.setRange(TILE_MIN, TILE_MAX)
        self._size.setValue(TILE_DEFAULT)
        self._size.setFixedWidth(120)
        self._size.setToolTip("Thumbnail size")
        self._size.valueChanged.connect(self._set_tile_size)

        # Any mix of categories, which the tree can not do, it holds one at a time
        self._filter_button = QToolButton()
        self._filter_button.setObjectName("FilterButton")
        self._filter_button.setText("Filters  \u25be")
        self._filter_button.setToolTip("Which categories to show")
        self._filter_button.setPopupMode(QToolButton.InstantPopup)
        self._filter_menu = QMenu(self)
        self._filter_actions = {}
        for kind, color in KIND_COLORS.items():
            action = QAction(kind, self._filter_menu, checkable=True, checked=True)
            action.setIcon(_swatch(color))
            action.toggled.connect(lambda checked, kind=kind: self._on_filter(kind, checked))
            self._filter_menu.addAction(action)
            self._filter_actions[kind] = action
        self._filter_menu.addSeparator()
        self._filter_menu.addAction("Show All", lambda: self._set_kinds(set(KIND_COLORS)))
        self._filter_button.setMenu(self._filter_menu)

        header = QFrame()
        header.setObjectName("BrowserHeader")
        row = QHBoxLayout(header)
        row.setContentsMargins(6, 4, 6, 4)
        row.addWidget(self._filter_button)
        row.addWidget(self._search, 1)
        row.addSpacing(12)
        row.addWidget(QLabel("Size"))
        row.addWidget(self._size)

        self._tree = QTreeWidget()
        self._tree.setHeaderHidden(True)
        self._tree.setIndentation(14)
        self._tree.setMinimumWidth(130)
        self._root = QTreeWidgetItem(["Engine"])
        self._tree_items = {}
        for kind in KIND_COLORS:
            child = QTreeWidgetItem([kind])
            child.setIcon(0, _swatch(KIND_COLORS[kind]))
            self._root.addChild(child)
            self._tree_items[kind] = child
        self._tree.addTopLevelItem(self._root)
        self._tree.expandAll()
        self._tree.setCurrentItem(self._root)
        self._tree.currentItemChanged.connect(self._on_category)

        self._delegate = TileDelegate(self)
        self._list = TileList()
        self._list.setObjectName("ContentTiles")
        self._list.setViewMode(QListView.IconMode)
        self._list.setResizeMode(QListView.Adjust)
        self._list.setMovement(QListView.Static)
        self._list.setWrapping(True)
        self._list.setUniformItemSizes(True)
        self._list.setSpacing(0)
        self._list.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self._list.setMouseTracking(True)
        self._list.setDragEnabled(True)  # onto the viewport, see TileList.startDrag
        self._list.setDragDropMode(QAbstractItemView.DragOnly)
        self._list.setContextMenuPolicy(Qt.CustomContextMenu)
        self._list.setItemDelegate(self._delegate)
        self._list.setGridSize(self._delegate.cell())
        self._list.itemDoubleClicked.connect(lambda item: self._add([item]))
        self._list.addRequested.connect(self._add)
        self._list.customContextMenuRequested.connect(self._context_menu)
        self._list.itemSelectionChanged.connect(self._update_footer)

        split = QSplitter(Qt.Horizontal)
        split.addWidget(self._tree)
        split.addWidget(self._list)
        split.setStretchFactor(1, 1)
        split.setSizes([150, 900])
        split.setChildrenCollapsible(False)

        self._footer = QLabel("")
        self._footer.setObjectName("Dim")
        self._footer.setContentsMargins(8, 3, 8, 3)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(header)
        layout.addWidget(split, 1)
        layout.addWidget(self._footer)

        self.setEnabled(False)

    # Engine

    def set_renderer(self, renderer):
        self._renderer = renderer
        self._fill()
        self.setEnabled(True)

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive or (self._object_target is None and self._light_target is None):
            return

        if self._object_target is not None and renderer.object_count >= self._object_target:
            # The newest object is the one that was added
            renderer.selected = renderer.object_count - 1
            self._object_target = None

        if self._light_target is not None and renderer.light_count >= self._light_target:
            self._light_target = None

        if self._object_target is None and self._light_target is None:
            self._update_footer(self._announce)
        elif time.monotonic() > self._deadline:
            # The engine dropped it, a model that would not load, or the scene filled up in between
            self._object_target = self._light_target = None
            self._update_footer("Nothing was added, the engine did not take the request")

    def objects_deleted(self, count):
        """Objects were removed from the scene. An addition still on its way is waited for at the new count."""
        if self._object_target is not None:
            self._object_target = max(0, self._object_target - count)

    # Entries

    def _fill(self):
        dxi = self._dxi
        self._list.clear()
        entries = []

        for name, mesh in (("Cube", dxi.SpawnMesh.Cube), ("Plane", dxi.SpawnMesh.Plane), ("Sphere", dxi.SpawnMesh.Sphere)):
            entries.append(Entry(
                name, "Shapes",
                lambda name=name: thumbnails.shape_thumbnail(name),
                lambda r, at, mesh=mesh: r.spawn_object(mesh, at=at),
                f"{name}  |  built in shape"))

        for index, (name, path) in enumerate(self._renderer.catalog_models()):
            entries.append(Entry(
                name, "Models",
                lambda path=path: thumbnails.model_thumbnail(path),
                lambda r, at, index=index: r.spawn_object(dxi.SpawnMesh.Model, index, at=at),
                f"{name}  |  model\n{path}"))

        for index, (name, folder, lone_map, _) in enumerate(self._renderer.catalog_materials()):
            diffuse = os.path.join(folder, name + "_diff_1k.png") if folder else lone_map
            entries.append(Entry(
                name, "Materials",
                lambda diffuse=diffuse: thumbnails.material_thumbnail(diffuse),
                None,
                f"{name}  |  material\nDouble click to put it on the selected object, or drag it onto an object",
                payload=index))

        for name, light in (("Directional", dxi.LightType.Directional), ("Point", dxi.LightType.Point),
                            ("Spot", dxi.LightType.Spot), ("Area", dxi.LightType.Area)):
            entries.append(Entry(
                f"{name} Light", "Lights",
                lambda name=name: thumbnails.light_thumbnail(name),
                lambda r, at, light=light: r.spawn_light(light, at=at),
                f"{name} Light  |  added at the camera"))

        for entry in entries:
            item = QListWidgetItem()
            item.setData(ENTRY, entry)
            item.setToolTip(entry.tooltip)
            self._list.addItem(item)

        self._apply_filter()

    def _items(self):
        return [self._list.item(i) for i in range(self._list.count())]

    # Filtering and size

    def _on_category(self, current, _previous):
        # A folder of the tree shows that category alone, the root, Engine, shows everything
        if current is None:
            return
        self._set_kinds({current.text(0)} if current.parent() is not None else set(KIND_COLORS))

    def _on_filter(self, kind, checked):
        kinds = set(self._kinds)
        kinds.add(kind) if checked else kinds.discard(kind)
        self._set_kinds(kinds)

    def _set_kinds(self, kinds):
        """The one place the categories on show are set. The tree and the Filters menu follow it."""
        self._kinds = set(kinds)

        for kind, action in self._filter_actions.items():
            set_quietly(action, action.setChecked, kind in self._kinds)

        # The tree shows the folder when exactly one category is on, Engine for all, and nothing for a mix
        if len(self._kinds) == 1:
            set_quietly(self._tree, self._tree.setCurrentItem, self._tree_items[next(iter(self._kinds))])
        elif len(self._kinds) == len(KIND_COLORS):
            set_quietly(self._tree, self._tree.setCurrentItem, self._root)
        else:
            set_quietly(self._tree, self._tree.setCurrentIndex, QModelIndex())
            self._tree.clearSelection()

        # The button says when something is filtered out, which a closed menu does not
        self._filter_button.setProperty("active", len(self._kinds) < len(KIND_COLORS))
        self._filter_button.style().unpolish(self._filter_button)
        self._filter_button.style().polish(self._filter_button)

        self._apply_filter()

    def _apply_filter(self, *_):
        needle = self._search.text().strip().lower()
        for item in self._items():
            entry = item.data(ENTRY)
            item.setHidden(entry.kind not in self._kinds or needle not in entry.name.lower())
        self._update_footer()

    def _set_tile_size(self, side):
        self._delegate.side = side
        self._list.setGridSize(self._delegate.cell())
        self._list.doItemsLayout()
        self._list.viewport().update()

    def _update_footer(self, message=""):
        if not isinstance(message, str):
            message = ""

        total = self._list.count()
        shown = sum(1 for item in self._items() if not item.isHidden())
        selected = len(self._list.selectedItems())
        text = (f"{shown} of {total} items" if shown != total else f"{shown} items") + (f" ({selected} selected)" if selected else "")
        self._footer.setText(f"{text}     {message}" if message else text)

    # Adding

    def _context_menu(self, position):
        item = self._list.itemAt(position)
        if item is None:
            return

        if not item.isSelected():
            self._list.clearSelection()
            item.setSelected(True)

        menu = QMenu(self)
        if item.data(ENTRY).kind == "Materials":
            menu.addAction("Assign to Selected Object", lambda: self._add(self._list.selectedItems()))
        else:
            menu.addAction("Add to Scene", lambda: self._add(self._list.selectedItems()))
        menu.exec(self._list.viewport().mapToGlobal(position))

    def drop_rows(self, rows, x, y):
        """Tiles dropped on the viewport at a pixel of it. Rows that are no longer there, from a refilled list, are skipped."""
        items = [self._list.item(row) for row in rows if 0 <= row < self._list.count()]
        self._add(items, at=(x, y))

    def _add(self, items, at=None):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        # Materials are worn, not added. Of several only the first can be, an object has one.
        materials = [item for item in items if item.data(ENTRY).kind == "Materials"]
        items = [item for item in items if item.data(ENTRY).kind != "Materials"]
        assigned = self._assign(materials[0], at) if materials else ""

        added, refused = [], ""
        for number, item in enumerate(items):
            entry = item.data(ENTRY)

            if entry.kind == "Lights":
                base = renderer.light_count if self._light_target is None else self._light_target
                if base >= self._dxi.MAX_LIGHTS:
                    refused = f"All {self._dxi.MAX_LIGHTS} lights of the scene are in use"
                    continue
                self._light_target = base + 1
            else:
                base = renderer.object_count if self._object_target is None else self._object_target
                if base >= self._dxi.MAX_OBJECTS:
                    refused = f"The scene is full, it holds {self._dxi.MAX_OBJECTS} objects"
                    continue
                self._object_target = base + 1

            # Several dropped together would land on one spot, so each is a little further along the row
            entry.add(renderer, None if at is None else (at[0] + number * DROP_SPREAD_PX, at[1]))
            added.append(entry.name)

        # What the footer says once the engine has caught up, so it is kept for refresh() to show
        parts = ([assigned] if assigned else []) + (["Added " + ", ".join(added)] if added else []) + ([refused] if refused else [])
        self._announce = ".  ".join(parts)

        if added:
            self._deadline = time.monotonic() + SPAWN_WAIT_S
        self._update_footer(self._announce)

    def _assign(self, item, at=None, object_index=None):
        """Puts a material tile on an object: the one given, else the one under a pixel of the view, else the
        selected one. Returns what the footer should say."""
        renderer = self._renderer
        entry = item.data(ENTRY)

        if object_index is not None:
            renderer.assign_material(object_index, entry.payload)
            return f"{entry.name} put on {renderer.object_names()[object_index]}"

        if at is not None:
            renderer.assign_material_at(entry.payload, at)
            return f"{entry.name} put on the object under the cursor"

        if renderer.selected < 0:
            return "Select an object first, a material is put on the selected object"

        renderer.assign_material(renderer.selected, entry.payload)
        return f"{entry.name} put on {renderer.object_names()[renderer.selected]}"

    def assign_rows(self, object_index, rows):
        """Material tiles dropped on an object's own tab. Tiles that are not materials are of no use there."""
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        for row in rows:
            item = self._list.item(row) if 0 <= row < self._list.count() else None
            if item is not None and item.data(ENTRY).kind == "Materials":
                self._announce = self._assign(item, object_index=object_index)
                self._update_footer(self._announce)
                return
