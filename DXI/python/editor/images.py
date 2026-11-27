"""Reading texture files for the editor's own pictures: a thumbnail of a material, the preview of a mesh.

The engine loads its textures on the GPU and does not hand them back, so the editor reads the same files itself.
"""

import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtGui import QImage

_cache = {}


def load_image(path):
    """The file as a QImage, or None when it is missing or not an image"""
    image = QImage(path)
    return None if image.isNull() else image


def image_size(path):
    """(width, height) of an image file without keeping it, or None"""
    image = load_image(path)
    return None if image is None else (image.width(), image.height())


def to_array(image):
    """A QImage as an (h, w, 3) uint8 array of its colour"""
    image = image.convertToFormat(QImage.Format_RGB888)
    width, height = image.width(), image.height()
    rows = np.frombuffer(image.constBits(), dtype=np.uint8, count=image.bytesPerLine() * height)
    return rows.reshape(height, image.bytesPerLine())[:, :width * 3].reshape(height, width, 3).copy()


def load_rgb(path, max_side=512):
    """The colour of a texture file as an (h, w, 3) uint8 array no larger than max_side, or None. Kept, since
    a thumbnail and the preview of the same material ask for it."""
    key = (path, max_side)
    if key not in _cache:
        image = load_image(path)
        if image is not None and max(image.width(), image.height()) > max_side:
            image = image.scaled(max_side, max_side, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        _cache[key] = None if image is None else to_array(image)
    return _cache[key]


def array_to_image(pixels):
    """An (h, w, 3 or 4) uint8 array as a QImage that owns its data"""
    pixels = np.ascontiguousarray(pixels)
    height, width, channels = pixels.shape
    form = QImage.Format_RGBA8888 if channels == 4 else QImage.Format_RGB888
    return QImage(pixels.data, width, height, width * channels, form).copy()
