"""Small widgets the panels are built from: a collapsible section, a labelled X Y Z row, and a binder that ties
a widget to one setting of the engine."""

from PySide6.QtCore import QSignalBlocker, Qt
from PySide6.QtWidgets import (
    QAbstractSpinBox, QCheckBox, QComboBox, QDoubleSpinBox, QFormLayout, QHBoxLayout, QLabel,
    QSizePolicy, QToolButton, QVBoxLayout, QWidget,
)

# Red, green, blue, as the engine's own gizmo and the Unreal editor draw the three axes
AXIS_COLORS = ("#cc3d3d", "#4fa34f", "#3d77cc")


def set_quietly(widget, setter, value):
    """Updates a widget without it announcing a change the user did not make"""
    with QSignalBlocker(widget):
        setter(value)


def is_editing(box):
    """Whether a spin box has the keyboard, so a refresh must leave what is typed in it alone"""
    return box.hasFocus() or box.lineEdit().hasFocus()


def make_spin(minimum, maximum, step, decimals=3):
    box = QDoubleSpinBox()
    box.setRange(minimum, maximum)
    box.setSingleStep(step)
    box.setDecimals(decimals)
    box.setKeyboardTracking(True)  # every digit applies, an object follows as the value is typed
    box.setButtonSymbols(QAbstractSpinBox.NoButtons)  # arrows take room a row of three does not have
    box.setAccelerated(True)
    return box


def make_vec3_row(boxes):
    """Three boxes, each behind a coloured X, Y or Z tag"""
    row = QWidget()
    layout = QHBoxLayout(row)
    layout.setContentsMargins(0, 0, 0, 0)
    layout.setSpacing(3)

    for axis, box, color in zip("XYZ", boxes, AXIS_COLORS):
        tag = QLabel(axis)
        tag.setFixedWidth(16)
        tag.setAlignment(Qt.AlignCenter)
        tag.setStyleSheet(f"background: {color}; color: #fff; font-weight: bold; border-radius: 2px; padding: 2px 0;")
        layout.addWidget(tag)
        layout.addWidget(box, 1)

    return row


class Section(QWidget):
    """A titled group that folds away, like a category in Unreal's details panel"""

    def __init__(self, title, expanded=True, parent=None):
        super().__init__(parent)
        self._title = title

        self._header = QToolButton()
        self._header.setObjectName("SectionHeader")
        self._header.setCheckable(True)
        self._header.setToolButtonStyle(Qt.ToolButtonTextOnly)
        self._header.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self._header.setFocusPolicy(Qt.NoFocus)
        self._header.toggled.connect(self._on_toggled)

        self._body = QWidget()
        self.form = QFormLayout(self._body)
        self.form.setContentsMargins(10, 8, 10, 8)
        self.form.setHorizontalSpacing(10)
        self.form.setVerticalSpacing(6)
        self.form.setLabelAlignment(Qt.AlignLeft | Qt.AlignVCenter)
        self.form.setFieldGrowthPolicy(QFormLayout.ExpandingFieldsGrow)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._header)
        layout.addWidget(self._body)

        self._header.setChecked(expanded)
        self._on_toggled(expanded)

    def _on_toggled(self, expanded):
        self._header.setText(("\u25be   " if expanded else "\u25b8   ") + self._title)
        self._body.setVisible(expanded)

    def add_row(self, label, field=None):
        """A labelled row, or with one argument a row that spans the width"""
        if field is None:
            self.form.addRow(label)
        else:
            self.form.addRow(label, field)


class Binder:
    """Ties widgets to settings of the engine, in both directions.

    Each widget names the setting it edits, as a function from the renderer to the settings object and an
    attribute. A change in the widget is written to the engine at once, and refresh() reads every setting
    back, since the engine changes some on its own, a hotkey switches the tool for one.
    """

    def __init__(self):
        self._renderer = None
        self._entries = []  # (widget, read, write-to-widget)

    def bind(self, renderer):
        self._renderer = renderer
        self.refresh()

    def _write(self, resolve, attr, value):
        if self._renderer is not None and self._renderer.alive:
            setattr(resolve(self._renderer), attr, value)

    def check(self, text, resolve, attr):
        box = QCheckBox(text)
        box.toggled.connect(lambda value: self._write(resolve, attr, value))
        self._entries.append((box, lambda r: getattr(resolve(r), attr), box.setChecked))
        return box

    def spin(self, resolve, attr, minimum, maximum, step, decimals=3):
        box = make_spin(minimum, maximum, step, decimals)
        box.valueChanged.connect(lambda value: self._write(resolve, attr, value))
        self._entries.append((box, lambda r: getattr(resolve(r), attr), box.setValue))
        return box

    def combo(self, resolve, attr, options):
        """options is a list of (label, engine value). Qt cannot compare the engine's enums as item data,
        so the position in the list is what stands for one."""
        box = QComboBox()
        box.addItems([label for label, _ in options])
        values = [value for _, value in options]
        box.currentIndexChanged.connect(lambda index: self._write(resolve, attr, values[index]))
        self._entries.append((box, lambda r: values.index(getattr(resolve(r), attr)), box.setCurrentIndex))
        return box

    def refresh(self):
        renderer = self._renderer
        if renderer is None or not renderer.alive:
            return

        for widget, read, show in self._entries:
            if isinstance(widget, QDoubleSpinBox) and is_editing(widget):
                continue
            set_quietly(widget, show, read(renderer))
