"""The outliner: every object and light of the scene, which the user can sort into folders.

Three columns, as Unreal's has them: the name, what it is, and for a light its intensity and where it is. Hover a
light for all of it. A folder is made with the + Folder button or the context menu (also from the selection), objects
and lights go into it by dragging or Move To, and its arrow winds it up or down. F2 renames a folder, Delete removes
it and puts what it held back at the top level. Delete on a selected object removes it from the scene, for good: there
is no undo. The panel only asks, the window carries it out (see main_window.py), since tabs and lists elsewhere
number their objects too.

Folders belong to the editor and not to the engine, which only knows its numbered objects. They are kept here for the
session: which folder holds which object is a list of (kind, engine index) pairs, and a folder can not nest. The engine
changes on its own (a spawn, a click in the view), so the tree is refreshed from it on the panels' timer and rebuilt
only when something differs.
"""

from dataclasses import dataclass

from PySide6.QtCore import QItemSelectionModel, QSignalBlocker, QTimer, Qt, Signal
from PySide6.QtGui import QColor, QIcon, QPainter, QPixmap
from PySide6.QtWidgets import (
    QAbstractItemView, QHBoxLayout, QInputDialog, QLabel, QLineEdit, QMenu, QStyle, QToolButton, QTreeWidget,
    QTreeWidgetItem, QVBoxLayout, QWidget,
)

KIND = Qt.UserRole        # "object", "light" or "folder"
REF = Qt.UserRole + 1     # the engine's index of an object or light, the id of a folder

OBJECT, LIGHT, FOLDER = "object", "light", "folder"
NEW_FOLDER_NAME = "New Folder"


@dataclass
class Folder:
    id: int
    name: str
    expanded: bool = True


def shifted(index, removed):
    """Where an object's index is after the objects in removed (a set of indices) are deleted: each removed one
    below it moves it up by one"""
    return index - sum(1 for gone in removed if gone < index)


def _dot(color, round_shape):
    """A small coloured mark in front of a name: a circle for a light in its own colour, a square for an object"""
    pixmap = QPixmap(14, 14)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.Antialiasing)
    painter.setPen(Qt.NoPen)
    painter.setBrush(color)
    if round_shape:
        painter.drawEllipse(2, 2, 10, 10)
    else:
        painter.drawRoundedRect(2, 2, 10, 10, 2, 2)
    painter.end()
    return QIcon(pixmap)


def light_color(rgb):
    """The colour of a light as a swatch shows it. Linear values can exceed 1, a swatch can not."""
    return QColor.fromRgbF(*(max(0.0, min(1.0, float(v))) for v in rgb))


def describe_light(info):
    """(type name, one line of info, tooltip) of what light_info returns"""
    name, kind, position, direction, color, intensity = info
    kind_name = getattr(kind, "name", str(kind))

    where = f"({position[0]:.1f}, {position[1]:.1f}, {position[2]:.1f})"
    facing = f"({direction[0]:.2f}, {direction[1]:.2f}, {direction[2]:.2f})"
    summary = f"I {intensity:g}   dir {facing}" if kind_name == "Directional" else f"I {intensity:g}   at {where}"

    lines = [f"{name}", f"{kind_name} light", f"Intensity  {intensity:g}", f"Colour  ({color[0]:.2f}, {color[1]:.2f}, {color[2]:.2f})"]
    if kind_name != "Directional":
        lines.append(f"Position  {where}")
    if kind_name in ("Directional", "Spot", "Area"):
        lines.append(f"Direction  {facing}")
    return f"{kind_name} Light", summary, "\n".join(lines)


class OutlinerTree(QTreeWidget):
    """The tree. A drop is not carried out by Qt, which would move rows: the panel decides where things go and
    rebuilds, so the drop only reports which folder it landed in."""

    moveRequested = Signal(list, object)  # (kind, index) of the dragged items, the folder id or None for the top level
    deleteRequested = Signal()

    def _folder_at(self, position):
        item = self.itemAt(position)
        if item is None:
            return None
        if item.data(0, KIND) == FOLDER:
            return item.data(0, REF)
        return item.parent().data(0, REF) if item.parent() is not None else None

    def selected_keys(self):
        return [(item.data(0, KIND), item.data(0, REF)) for item in self.selectedItems() if item.data(0, KIND) != FOLDER]

    def dragEnterEvent(self, event):
        if event.source() is self:
            event.acceptProposedAction()

    def dragMoveEvent(self, event):
        if event.source() is self:
            event.acceptProposedAction()

    def dropEvent(self, event):
        if event.source() is not self:
            return

        keys = self.selected_keys()
        folder = self._folder_at(event.position().toPoint())

        event.setDropAction(Qt.CopyAction)
        event.accept()

        if keys:
            # After the drag has finished, the tree is about to be rebuilt
            QTimer.singleShot(0, lambda: self.moveRequested.emit(keys, folder))

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Delete:
            self.deleteRequested.emit()
            return
        super().keyPressEvent(event)


class OutlinerPanel(QWidget):
    openRequested = Signal(int)  # a double click or the context menu: the object's own tab
    deleteObjectsRequested = Signal(list)  # Delete or the context menu: indices of the objects to remove from the scene

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("Panel")
        self._renderer = None

        self._folders = {}          # id -> Folder, in the order they were made
        self._next_id = 1
        self._filed = {}            # (kind, engine index) -> folder id, what is not here is at the top level
        self._signature = None      # what the engine listed when the tree was last built
        self._counts = (0, 0)       # objects, lights
        self._engine_selected = -2  # what the engine had selected when the tree last followed it
        self._objects = {}          # engine index -> its row

        self._search = QLineEdit()
        self._search.setPlaceholderText("Search...")
        self._search.setClearButtonEnabled(True)
        self._search.textChanged.connect(self._apply_filter)

        add_folder = QToolButton()
        add_folder.setText("+ Folder")
        add_folder.setToolTip("Make a folder. With objects selected they go into it.")
        add_folder.clicked.connect(lambda: self._ask_new_folder(self._tree.selected_keys()))

        self._tree = OutlinerTree()
        self._tree.setObjectName("OutlinerTree")
        self._tree.setColumnCount(3)
        self._tree.setHeaderLabels(["Label", "Type", "Info"])
        self._tree.header().setStretchLastSection(True)
        self._tree.header().resizeSection(0, 150)
        self._tree.header().resizeSection(1, 100)
        # What does not fit is cut short, a light's tooltip has all of it
        self._tree.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self._tree.setAlternatingRowColors(True)
        self._tree.setIndentation(16)
        self._tree.setUniformRowHeights(True)
        self._tree.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self._tree.setEditTriggers(QAbstractItemView.EditKeyPressed)   # F2, and only a folder's name can be edited
        self._tree.setDragEnabled(True)
        self._tree.setAcceptDrops(True)
        self._tree.setDropIndicatorShown(True)
        # A copy, not Qt's internal move: after a move the view deletes the rows it dragged, and the panel decides
        # where things go and rebuilds the tree itself
        self._tree.setDragDropMode(QAbstractItemView.DragDrop)
        self._tree.setDefaultDropAction(Qt.CopyAction)
        self._tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self._tree.currentItemChanged.connect(self._on_current)
        self._tree.itemSelectionChanged.connect(self._update_count)
        self._tree.itemDoubleClicked.connect(self._on_double_click)
        self._tree.itemExpanded.connect(lambda item: self._on_expanded(item, True))
        self._tree.itemCollapsed.connect(lambda item: self._on_expanded(item, False))
        self._tree.itemChanged.connect(self._on_renamed)
        self._tree.customContextMenuRequested.connect(self._context_menu)
        self._tree.moveRequested.connect(self.move_to_folder)
        self._tree.deleteRequested.connect(self._delete_selection)

        self._count = QLabel("")
        self._count.setObjectName("Dim")

        top = QHBoxLayout()
        top.setSpacing(6)
        top.addWidget(self._search, 1)
        top.addWidget(add_folder)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 6, 6, 4)
        layout.setSpacing(6)
        layout.addLayout(top)
        layout.addWidget(self._tree, 1)
        layout.addWidget(self._count)

    # Engine

    def set_renderer(self, renderer):
        self._renderer = renderer
        self.refresh()

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        names = renderer.object_names()
        lights = [renderer.light_info(i) for i in range(renderer.light_count)]

        signature = (tuple(names), tuple((info[0], getattr(info[1], "name", str(info[1])),
                                          tuple(round(v, 3) for v in info[2]), tuple(round(v, 3) for v in info[3]),
                                          tuple(round(v, 3) for v in info[4]), round(info[5], 3)) for info in lights))
        if signature != self._signature:
            self._signature = signature
            self._counts = (len(names), len(lights))
            self._prune()
            self._rebuild(names, lights)

        self.show_selection(renderer.selected)

    def _prune(self):
        """What the engine no longer has can not be in a folder. It never removes objects, but a scene can be reloaded."""
        objects, lights = self._counts
        self._filed = {key: folder for key, folder in self._filed.items()
                       if key[1] < (objects if key[0] == OBJECT else lights) and folder in self._folders}

    # Building

    def _rebuild(self, names=None, lights=None):
        if names is None:
            names = self._renderer.object_names()
            lights = [self._renderer.light_info(i) for i in range(self._renderer.light_count)]

        selected = {(item.data(0, KIND), item.data(0, REF)) for item in self._tree.selectedItems()}
        current = self._tree.currentItem()
        current_key = None if current is None else (current.data(0, KIND), current.data(0, REF))

        self._objects = {}
        folder_items = {}

        with QSignalBlocker(self._tree):
            self._tree.clear()

            for folder in self._folders.values():
                item = QTreeWidgetItem([folder.name, "Folder", ""])
                item.setData(0, KIND, FOLDER)
                item.setData(0, REF, folder.id)
                item.setIcon(0, self.style().standardIcon(QStyle.SP_DirIcon))
                item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable | Qt.ItemIsDropEnabled)
                self._tree.addTopLevelItem(item)
                folder_items[folder.id] = item

            def place(item, key):
                folder = self._filed.get(key)
                if folder in folder_items:
                    folder_items[folder].addChild(item)
                else:
                    self._tree.addTopLevelItem(item)

            for index, name in enumerate(names):
                item = QTreeWidgetItem([name, "Mesh", ""])
                item.setData(0, KIND, OBJECT)
                item.setData(0, REF, index)
                item.setIcon(0, _dot(QColor("#8a8a8a"), round_shape=False))
                item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)
                place(item, (OBJECT, index))
                self._objects[index] = item

            for index, info in enumerate(lights):
                kind_text, summary, tooltip = describe_light(info)
                item = QTreeWidgetItem([info[0], kind_text, summary])
                item.setData(0, KIND, LIGHT)
                item.setData(0, REF, index)
                item.setIcon(0, _dot(light_color(info[4]), round_shape=True))
                item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)
                for column in range(3):
                    item.setToolTip(column, tooltip)
                place(item, (LIGHT, index))

            for folder_id, item in folder_items.items():
                item.setText(2, self._folder_summary(item))

            self._restore_selection(selected, current_key)

        self._apply_filter()
        self._update_count()

    @staticmethod
    def _folder_summary(item):
        count = item.childCount()
        return "empty" if count == 0 else f"{count} item" + ("" if count == 1 else "s")

    def _restore_selection(self, selected, current_key):
        for item in self._all_items():
            key = (item.data(0, KIND), item.data(0, REF))
            if key in selected:
                item.setSelected(True)
            if key == current_key:
                # Current without touching what is selected
                self._tree.selectionModel().setCurrentIndex(self._tree.indexFromItem(item), QItemSelectionModel.NoUpdate)

    def _all_items(self):
        for top in range(self._tree.topLevelItemCount()):
            item = self._tree.topLevelItem(top)
            yield item
            for child in range(item.childCount()):
                yield item.child(child)

    def _folder_items(self):
        return [item for item in self._all_items() if item.data(0, KIND) == FOLDER]

    # Folders

    def _unique_name(self, wanted, ignore=None):
        taken = {folder.name.lower() for folder in self._folders.values() if folder.id != ignore}
        name, number = wanted, 2
        while name.lower() in taken:
            name = f"{wanted} {number}"
            number += 1
        return name

    def create_folder(self, name=NEW_FOLDER_NAME, keys=()):
        """Makes a folder, with these (kind, index) items in it. Returns its id."""
        name = (name or "").strip() or NEW_FOLDER_NAME
        folder = Folder(self._next_id, self._unique_name(name))
        self._next_id += 1
        self._folders[folder.id] = folder
        for key in keys:
            self._filed[tuple(key)] = folder.id
        self._rebuild()
        return folder.id

    def _ask_new_folder(self, keys):
        name, accepted = QInputDialog.getText(self, "New Folder", "Name:", text=self._unique_name(NEW_FOLDER_NAME))
        if accepted:
            self.create_folder(name, keys)

    def move_to_folder(self, keys, folder_id):
        """Puts (kind, index) items in a folder, or at the top level for None"""
        if folder_id is not None and folder_id not in self._folders:
            return
        for key in keys:
            if folder_id is None:
                self._filed.pop(tuple(key), None)
            else:
                self._filed[tuple(key)] = folder_id
        self._rebuild()

    def delete_folder(self, folder_id):
        """The folder goes, what it held stays in the scene at the top level"""
        if self._folders.pop(folder_id, None) is None:
            return
        self._filed = {key: folder for key, folder in self._filed.items() if folder != folder_id}
        self._rebuild()

    def folders(self):
        """(name, expanded, [(kind, index), ...]) of each folder, for whoever wants to look"""
        return [(folder.name, folder.expanded, sorted(key for key, owner in self._filed.items() if owner == folder.id))
                for folder in self._folders.values()]

    def set_all_expanded(self, expanded):
        for folder in self._folders.values():
            folder.expanded = expanded
        with QSignalBlocker(self._tree):
            for item in self._folder_items():
                item.setExpanded(expanded)
        self._apply_filter()

    def _delete_selection(self):
        """Delete: a selected folder goes and keeps what it held, a selected object is removed from the scene. A
        light stays, the engine has no way to take one out."""
        # Read everything first: deleting a folder rebuilds the tree and frees the items
        keys = [(item.data(0, KIND), item.data(0, REF)) for item in self._tree.selectedItems()]
        for kind, ref in keys:
            if kind == FOLDER:
                self.delete_folder(ref)

        objects = sorted({ref for kind, ref in keys if kind == OBJECT})
        if objects:
            self.deleteObjectsRequested.emit(objects)

    def objects_deleted(self, removed):
        """The engine removed these objects (a set of indices), the ones behind them moved up. What is filed
        follows, and the tree is rebuilt from the engine."""
        removed = set(removed)
        self._filed = {(kind, shifted(index, removed) if kind == OBJECT else index): folder
                       for (kind, index), folder in self._filed.items()
                       if not (kind == OBJECT and index in removed)}

        # What was selected was named by the old indices, it would land on a neighbour
        with QSignalBlocker(self._tree):
            self._tree.clearSelection()
            self._tree.setCurrentItem(None)
        self._signature = None
        self._engine_selected = -2
        self.refresh()

    def _on_expanded(self, item, expanded):
        if item.data(0, KIND) == FOLDER and not self._search.text().strip():
            folder = self._folders.get(item.data(0, REF))
            if folder is not None:
                folder.expanded = expanded

    def _on_renamed(self, item, column):
        if column != 0 or item.data(0, KIND) != FOLDER:
            return

        folder = self._folders.get(item.data(0, REF))
        if folder is None:
            return

        text = item.text(0).strip()
        folder.name = self._unique_name(text, ignore=folder.id) if text else folder.name
        with QSignalBlocker(self._tree):
            item.setText(0, folder.name)

    # Selection

    def _on_current(self, current, _previous):
        """A click on an object selects it in the engine, a click on a light selects no object, a folder is the
        outliner's alone"""
        renderer = self._renderer
        if current is None or renderer is None or not renderer.alive:
            return

        kind = current.data(0, KIND)
        if kind == OBJECT:
            self._engine_selected = current.data(0, REF)
            renderer.selected = self._engine_selected
        elif kind == LIGHT:
            self._engine_selected = -1
            renderer.selected = -1

    def show_selection(self, index):
        """Follows the engine's selection when that changed: a click in the view, Escape, a spawn"""
        if index == self._engine_selected:
            return
        self._engine_selected = index

        with QSignalBlocker(self._tree):
            if index >= 0 and index in self._objects:
                item = self._objects[index]
                parent = item.parent()
                if parent is not None and not parent.isExpanded():
                    # A selected object is not left inside a folder that is wound up
                    parent.setExpanded(True)
                    folder = self._folders.get(parent.data(0, REF))
                    if folder is not None:
                        folder.expanded = True
                self._tree.clearSelection()
                self._tree.setCurrentItem(item)
            elif index < 0:
                current = self._tree.currentItem()
                if current is not None and current.data(0, KIND) == OBJECT:
                    self._tree.clearSelection()
                    self._tree.setCurrentItem(None)
        self._update_count()

    def _on_double_click(self, item, _column):
        if item.data(0, KIND) == OBJECT:
            self.openRequested.emit(item.data(0, REF))

    def _update_count(self):
        objects, lights = self._counts
        picked = len([item for item in self._tree.selectedItems() if item.data(0, KIND) != FOLDER])
        text = f"{objects} objects, {lights} light" + ("" if lights == 1 else "s")
        if self._folders:
            text += f", {len(self._folders)} folder" + ("" if len(self._folders) == 1 else "s")
        self._count.setText(text + (f"  ({picked} selected)" if picked else ""))

    # Search

    def _apply_filter(self, *_):
        needle = self._search.text().strip().lower()

        def matches(item):
            return any(needle in item.text(column).lower() for column in range(3))

        with QSignalBlocker(self._tree):
            for top in range(self._tree.topLevelItemCount()):
                item = self._tree.topLevelItem(top)

                if item.data(0, KIND) != FOLDER:
                    item.setHidden(bool(needle) and not matches(item))
                    continue

                # A folder whose own name matches shows everything in it, otherwise only what matches
                whole = needle in item.text(0).lower()
                shown = 0
                for child in range(item.childCount()):
                    row = item.child(child)
                    visible = not needle or whole or matches(row)
                    row.setHidden(not visible)
                    shown += visible

                item.setHidden(bool(needle) and not (whole or shown))

                # While searching, a folder with hits is open so they can be seen. What the user had is put
                # back afterwards, the state is the folder's and a search must not change it.
                folder = self._folders[item.data(0, REF)]
                item.setExpanded(True if needle and shown else folder.expanded)

    # Context menu

    def _context_menu(self, position):
        item = self._tree.itemAt(position)
        if item is not None and not item.isSelected():
            self._tree.clearSelection()
            item.setSelected(True)

        menu = QMenu(self)
        keys = self._tree.selected_keys()
        folders = [it for it in self._tree.selectedItems() if it.data(0, KIND) == FOLDER]

        if item is not None and item.data(0, KIND) == OBJECT and len(keys) == 1:
            menu.addAction("Open in Tab", lambda index=item.data(0, REF): self.openRequested.emit(index))
            menu.addSeparator()

        if any(kind == OBJECT for kind, _ in keys):
            menu.addAction("Delete\tDel", self._delete_selection)
            menu.addSeparator()

        if keys:
            move = menu.addMenu("Move To")
            for folder in self._folders.values():
                move.addAction(folder.name, lambda folder_id=folder.id: self.move_to_folder(keys, folder_id))
            if self._folders:
                move.addSeparator()
            move.addAction("New Folder...", lambda: self._ask_new_folder(keys))

            if any(key in self._filed for key in keys):
                menu.addAction("Remove from Folder", lambda: self.move_to_folder(keys, None))
            menu.addAction("New Folder with Selection...", lambda: self._ask_new_folder(keys))
        else:
            menu.addAction("New Folder...", lambda: self._ask_new_folder([]))

        if len(folders) == 1:
            menu.addSeparator()
            target = folders[0]
            menu.addAction("Rename", lambda: self._tree.editItem(target, 0))
            menu.addAction("Delete Folder", lambda: self.delete_folder(target.data(0, REF)))

        if self._folders:
            menu.addSeparator()
            menu.addAction("Expand All Folders", lambda: self.set_all_expanded(True))
            menu.addAction("Collapse All Folders", lambda: self.set_all_expanded(False))

        menu.exec(self._tree.viewport().mapToGlobal(position))
