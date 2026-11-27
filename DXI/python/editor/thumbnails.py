"""Square thumbnails for the content browser.

A mesh is drawn from its own geometry: flat shaded triangles, far ones first, seen from the side and a little above,
the way a content browser shows a static mesh. That needs no GPU and no trip through the engine, so the browser has
its pictures before the first frame is rendered. A model that has a texture beside it is painted with it. A light has
no geometry, so it gets a drawn symbol.

Every thumbnail is a SIZE x SIZE pixmap, the browser scales it to the tile.
"""

import glob
import math
import os

import numpy as np
from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import (
    QBrush, QColor, QImage, QLinearGradient, QPainter, QPainterPath, QPen, QPixmap, QPolygonF, QRadialGradient,
)

import images

SIZE = 256

BACKGROUND_TOP    = QColor("#363636")
BACKGROUND_BOTTOM = QColor("#1c1c1c")
CLAY              = (0.74, 0.74, 0.76)  # a mesh without a texture of its own

# Where the light comes from, in the picture's own space: the camera looks along +Z, so a light on the camera's
# side has a negative Z. From the upper left, in front.
_LIGHT = (-0.45, 0.75, -0.5)

_cache = {}


# Meshes. A triangle is (a, b, c, color, normal): the corners as (x, y, z), a base colour as three 0-1 floats, and a
# normal of its own, or None for the one the three corners give.

def _quad(a, b, c, d, color):
    return [(a, b, c, color, None), (a, c, d, color, None)]


def _cube_triangles():
    h = 0.5
    p = [(x, y, z) for z in (-h, h) for y in (-h, h) for x in (-h, h)]  # index = x + 2y + 4z
    faces = ((0, 1, 3, 2), (4, 6, 7, 5), (0, 2, 6, 4), (1, 5, 7, 3), (0, 4, 5, 1), (2, 3, 7, 6))
    triangles = []
    for a, b, c, d in faces:
        triangles += _quad(p[a], p[b], p[c], p[d], CLAY)
    return triangles


def _plane_triangles(cells=8, size=1.5):
    # Laid flat, in a checker, or it would be a blank square
    triangles = []
    step = size / cells
    origin = -size / 2
    for i in range(cells):
        for j in range(cells):
            shade = 0.80 if (i + j) % 2 == 0 else 0.56
            x0, z0 = origin + i * step, origin + j * step
            triangles += _quad((x0, 0, z0), (x0, 0, z0 + step), (x0 + step, 0, z0 + step), (x0 + step, 0, z0),
                               (shade, shade, shade * 1.02))
    return triangles


def _sphere_triangles(bands=22):
    def point(lat, lon):
        y = math.cos(lat)
        r = math.sin(lat)
        return (r * math.cos(lon) * 0.5, y * 0.5, r * math.sin(lon) * 0.5)

    triangles = []
    for i in range(bands):
        lat0, lat1 = math.pi * i / bands, math.pi * (i + 1) / bands
        for j in range(bands * 2):
            lon0, lon1 = math.pi * j / bands, math.pi * (j + 1) / bands
            a, b, c, d = point(lat0, lon0), point(lat1, lon0), point(lat1, lon1), point(lat0, lon1)
            # The sphere's own normal at the middle of the face, which shades it smooth
            for tri in ((a, b, c), (a, c, d)):
                centre = [sum(v[k] for v in tri) for k in range(3)]
                triangles.append((*tri, CLAY, _normalized(centre)))
    return triangles


def _read_obj(path):
    """The triangles of an .obj, coloured from the texture beside it when it has uvs and there is one"""
    positions, uvs, faces = [], [], []

    with open(path, "r", errors="replace") as source:
        for line in source:
            parts = line.split()
            if not parts:
                continue
            if parts[0] == "v" and len(parts) >= 4:
                positions.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == "vt" and len(parts) >= 3:
                uvs.append((float(parts[1]), float(parts[2])))
            elif parts[0] == "f" and len(parts) >= 4:
                faces.append(parts[1:])

    def resolve(token, count):
        index = int(token)
        return index - 1 if index > 0 else count + index

    textures = sorted(glob.glob(os.path.join(os.path.dirname(path), "*.png")))
    texture = QImage(textures[0]) if textures and uvs else QImage()

    def colour_at(uv):
        if texture.isNull():
            return CLAY
        x = int((uv[0] % 1.0) * (texture.width() - 1))
        y = int((1.0 - (uv[1] % 1.0)) * (texture.height() - 1))
        c = texture.pixelColor(x, y)
        return (c.redF(), c.greenF(), c.blueF())

    triangles = []
    for face in faces:
        corners = []
        for token in face:
            fields = token.split("/")
            position = positions[resolve(fields[0], len(positions))]
            uv = uvs[resolve(fields[1], len(uvs))] if len(fields) > 1 and fields[1] and uvs else None
            corners.append((position, uv))

        for k in range(1, len(corners) - 1):  # a fan turns a polygon into triangles
            a, b, c = corners[0], corners[k], corners[k + 1]
            uv = None
            if a[1] and b[1] and c[1]:
                uv = ((a[1][0] + b[1][0] + c[1][0]) / 3, (a[1][1] + b[1][1] + c[1][1]) / 3)
            triangles.append((a[0], b[0], c[0], colour_at(uv) if uv else CLAY, None))

    return triangles


def _normalized(v):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) or 1.0
    return (v[0] / length, v[1] / length, v[2] / length)


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


# Drawing

def _canvas():
    pixmap = QPixmap(SIZE, SIZE)
    painter = QPainter(pixmap)
    painter.setRenderHints(QPainter.Antialiasing | QPainter.SmoothPixmapTransform)

    gradient = QLinearGradient(0, 0, 0, SIZE)
    gradient.setColorAt(0.0, BACKGROUND_TOP)
    gradient.setColorAt(1.0, BACKGROUND_BOTTOM)
    painter.fillRect(0, 0, SIZE, SIZE, gradient)
    return pixmap, painter


def _draw_mesh(triangles, yaw=-38.0, tilt=24.0, fill=0.74):
    """The triangles centred, scaled to fill a share of the picture, and drawn far to near"""
    pixmap, painter = _canvas()

    # Rotation about Y by yaw, then about X so the top turns towards the viewer
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    ct, st = math.cos(math.radians(tilt)), math.sin(math.radians(tilt))

    def rotate(v):
        x, y, z = v[0] * cy + v[2] * sy, v[1], -v[0] * sy + v[2] * cy
        return (x, y * ct + z * st, -y * st + z * ct)

    corners = [v for tri in triangles for v in tri[:3]]
    low = [min(v[k] for v in corners) for k in range(3)]
    high = [max(v[k] for v in corners) for k in range(3)]
    centre = [(low[k] + high[k]) / 2 for k in range(3)]
    radius = max(math.dist(v, centre) for v in corners) or 1.0
    scale = SIZE * fill / 2 / radius
    middle = SIZE / 2

    def project(v):
        r = rotate((v[0] - centre[0], v[1] - centre[1], v[2] - centre[2]))
        return (middle + r[0] * scale, middle - r[1] * scale, r[2])

    light = _normalized(_LIGHT)
    drawn = []
    for a, b, c, colour, normal in triangles:
        pa, pb, pc = project(a), project(b), project(c)

        if normal is None:
            ab = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
            ac = (c[0] - a[0], c[1] - a[1], c[2] - a[2])
            normal = _normalized(_cross(ab, ac))
        n = rotate(normal)

        # Either side of a face is lit, whichever way its winding runs, and the far ones are painted over
        facing = abs(n[0] * light[0] + n[1] * light[1] + n[2] * light[2])
        shade = 0.30 + 0.70 * facing
        depth = (pa[2] + pb[2] + pc[2]) / 3
        drawn.append((depth, pa, pb, pc, colour, shade))

    # A soft shadow under the object, where its lowest point is
    lowest = max(max(p[1] for p in (d[1], d[2], d[3])) for d in drawn)
    if high[1] - low[1] > 0.05:
        half = radius * scale * 0.85
        shadow = QRadialGradient(QPointF(middle, lowest), half)
        shadow.setColorAt(0.0, QColor(0, 0, 0, 150))
        shadow.setColorAt(1.0, QColor(0, 0, 0, 0))
        painter.setPen(Qt.NoPen)
        painter.setBrush(QBrush(shadow))
        painter.save()
        painter.translate(0, lowest)
        painter.scale(1.0, 0.22)
        painter.translate(0, -lowest)
        painter.drawEllipse(QPointF(middle, lowest), half, half)
        painter.restore()

    drawn.sort(key=lambda item: -item[0])
    for _, pa, pb, pc, colour, shade in drawn:
        fill_colour = QColor.fromRgbF(min(colour[0] * shade, 1.0), min(colour[1] * shade, 1.0), min(colour[2] * shade, 1.0))
        painter.setBrush(fill_colour)
        painter.setPen(QPen(fill_colour, 0.8))  # a thin edge in its own colour hides the seams between faces
        painter.drawPolygon(QPolygonF([QPointF(pa[0], pa[1]), QPointF(pb[0], pb[1]), QPointF(pc[0], pc[1])]))

    painter.end()
    return pixmap


# Lights

def _glow(painter, centre, radius, colour, alpha):
    gradient = QRadialGradient(centre, radius)
    inner = QColor(colour)
    inner.setAlpha(alpha)
    outer = QColor(colour)
    outer.setAlpha(0)
    gradient.setColorAt(0.0, inner)
    gradient.setColorAt(1.0, outer)
    painter.setPen(Qt.NoPen)
    painter.setBrush(QBrush(gradient))
    painter.drawEllipse(centre, radius, radius)


def _light_directional():
    pixmap, painter = _canvas()
    centre = QPointF(SIZE / 2, SIZE * 0.44)
    _glow(painter, centre, 110, QColor("#ffd45a"), 110)

    for i in range(12):
        angle = math.radians(i * 30)
        inner, outer = 58, 76 if i % 2 == 0 else 68
        painter.setPen(QPen(QColor("#ffd45a"), 7, Qt.SolidLine, Qt.RoundCap))
        painter.drawLine(QPointF(centre.x() + math.cos(angle) * inner, centre.y() + math.sin(angle) * inner),
                         QPointF(centre.x() + math.cos(angle) * outer, centre.y() + math.sin(angle) * outer))

    disc = QRadialGradient(centre, 44)
    disc.setColorAt(0.0, QColor("#fffbe0"))
    disc.setColorAt(1.0, QColor("#ffc233"))
    painter.setPen(Qt.NoPen)
    painter.setBrush(QBrush(disc))
    painter.drawEllipse(centre, 44, 44)

    # What it does to the scene: parallel rays all the same way
    painter.setPen(QPen(QColor(255, 220, 120, 190), 5, Qt.SolidLine, Qt.RoundCap))
    for x in (92, 128, 164):
        painter.drawLine(QPointF(x - 14, 176), QPointF(x + 6, 214))
    painter.end()
    return pixmap


def _light_point():
    pixmap, painter = _canvas()
    centre = QPointF(SIZE / 2, SIZE * 0.42)
    _glow(painter, centre, 118, QColor("#ffe27a"), 150)

    painter.setPen(Qt.NoPen)
    painter.setBrush(QColor("#6d6d70"))
    painter.drawRoundedRect(QRectF(SIZE / 2 - 17, SIZE * 0.42 + 36, 34, 26), 5, 5)
    painter.setBrush(QColor("#4b4b4e"))
    painter.drawRoundedRect(QRectF(SIZE / 2 - 12, SIZE * 0.42 + 62, 24, 12), 4, 4)

    bulb = QRadialGradient(QPointF(centre.x() - 10, centre.y() - 12), 52)
    bulb.setColorAt(0.0, QColor("#ffffff"))
    bulb.setColorAt(0.6, QColor("#ffeb99"))
    bulb.setColorAt(1.0, QColor("#ffc94a"))
    painter.setBrush(QBrush(bulb))
    painter.drawEllipse(centre, 40, 40)
    painter.end()
    return pixmap


def _light_spot():
    pixmap, painter = _canvas()
    top, floor = 78, 206

    beam = QPainterPath()
    beam.moveTo(104, top)
    beam.lineTo(152, top)
    beam.lineTo(214, floor)
    beam.lineTo(42, floor)
    beam.closeSubpath()
    gradient = QLinearGradient(0, top, 0, floor)
    gradient.setColorAt(0.0, QColor(255, 232, 140, 200))
    gradient.setColorAt(1.0, QColor(255, 232, 140, 30))
    painter.setPen(Qt.NoPen)
    painter.setBrush(QBrush(gradient))
    painter.drawPath(beam)

    painter.setBrush(QColor(255, 232, 140, 95))
    painter.drawEllipse(QPointF(SIZE / 2, floor), 86, 17)

    painter.setBrush(QColor("#6d6d70"))
    painter.drawRoundedRect(QRectF(96, 40, 64, 40), 8, 8)
    painter.setBrush(QColor("#4b4b4e"))
    painter.drawRoundedRect(QRectF(118, 24, 20, 20), 4, 4)
    painter.setBrush(QColor("#fff2b8"))
    painter.drawEllipse(QPointF(SIZE / 2, top - 2), 22, 9)
    painter.end()
    return pixmap


def _light_area():
    pixmap, painter = _canvas()
    colour = QColor("#8ab4ff")

    # The light a panel throws, widening to the floor
    spread = QPainterPath()
    spread.moveTo(60, 88)
    spread.lineTo(196, 88)
    spread.lineTo(230, 208)
    spread.lineTo(26, 208)
    spread.closeSubpath()
    gradient = QLinearGradient(0, 88, 0, 208)
    start = QColor(colour)
    start.setAlpha(170)
    end = QColor(colour)
    end.setAlpha(25)
    gradient.setColorAt(0.0, start)
    gradient.setColorAt(1.0, end)
    painter.setPen(Qt.NoPen)
    painter.setBrush(QBrush(gradient))
    painter.drawPath(spread)

    floor = QColor(colour)
    floor.setAlpha(80)
    painter.setBrush(floor)
    painter.drawEllipse(QPointF(SIZE / 2, 208), 100, 18)

    # The panel itself, drawn as seen from a little below
    panel = QPolygonF([QPointF(52, 60), QPointF(204, 60), QPointF(224, 92), QPointF(32, 92)])
    painter.setBrush(QColor("#4b4b4e"))
    painter.drawPolygon(QPolygonF([QPointF(48, 54), QPointF(208, 54), QPointF(228, 62), QPointF(28, 62)]))
    glass = QLinearGradient(0, 60, 0, 92)
    glass.setColorAt(0.0, QColor("#ffffff"))
    glass.setColorAt(1.0, colour)
    painter.setBrush(QBrush(glass))
    painter.drawPolygon(panel)
    painter.end()
    return pixmap


_LIGHTS = {
    "Directional": _light_directional,
    "Point":       _light_point,
    "Spot":        _light_spot,
    "Area":        _light_area,
}


# What the browser asks for. Each is made once.

def shape_thumbnail(kind):
    """kind is 'Cube', 'Plane' or 'Sphere'"""
    key = ("shape", kind)
    if key not in _cache:
        if kind == "Plane":
            _cache[key] = _draw_mesh(_plane_triangles(), tilt=36.0, fill=0.86)
        elif kind == "Sphere":
            _cache[key] = _draw_mesh(_sphere_triangles(), fill=0.72)
        else:
            _cache[key] = _draw_mesh(_cube_triangles(), fill=0.74)
    return _cache[key]


def model_thumbnail(path):
    """A model seen from its .obj. A file that can not be read gives a grey tile with a cross, never an error"""
    key = ("model", path)
    if key not in _cache:
        try:
            _cache[key] = _draw_mesh(_read_obj(path), fill=0.86)
        except (OSError, ValueError, IndexError):
            _cache[key] = _missing_thumbnail()
    return _cache[key]


def material_thumbnail(diffuse_path):
    """A sphere wearing the material's colour map, the way Unreal shows a material. A map that can not be read
    gives a grey sphere."""
    key = ("material", diffuse_path)
    if key not in _cache:
        _cache[key] = _material_sphere(images.load_rgb(diffuse_path, 256))
    return _cache[key]


def _material_sphere(texture):
    size = SIZE
    margin = 0.06 * size
    radius = size / 2 - margin

    ys, xs = np.mgrid[0:size, 0:size].astype(np.float32)
    nx = (xs + 0.5 - size / 2) / radius
    ny = -(ys + 0.5 - size / 2) / radius
    r2 = nx * nx + ny * ny
    nz = np.sqrt(np.clip(1.0 - r2, 0.0, 1.0))
    coverage = np.clip((1.0 - np.sqrt(r2)) * radius, 0.0, 1.0)[..., None]

    if texture is None:
        albedo = np.full((size, size, 3), 0.62, dtype=np.float32)
    else:
        # Longitude and latitude, laid over the sphere twice so a tiling map shows its repeat
        u = (np.arctan2(nx, nz) / (2 * np.pi) + 0.5) * 2.0
        v = np.arccos(np.clip(ny, -1.0, 1.0)) / np.pi
        h, w = texture.shape[:2]
        albedo = texture[(v * (h - 1)).astype(int) % h, (u * w).astype(int) % w].astype(np.float32) / 255.0

    light = np.array(_normalized(_LIGHT), dtype=np.float32) * np.array([1, 1, -1], dtype=np.float32)
    diffuse = np.clip(nx * light[0] + ny * light[1] + nz * light[2], 0.0, 1.0)
    half = _normalized((light[0], light[1], light[2] + 1.0))
    specular = np.clip(nx * half[0] + ny * half[1] + nz * half[2], 0.0, 1.0) ** 40 * 0.25
    lit = albedo * (0.22 + 0.78 * diffuse[..., None]) + specular[..., None]

    # Over the same gradient the other thumbnails have
    t = (np.arange(size, dtype=np.float32) / (size - 1))[:, None, None]
    top = np.array([BACKGROUND_TOP.redF(), BACKGROUND_TOP.greenF(), BACKGROUND_TOP.blueF()], dtype=np.float32)
    bottom = np.array([BACKGROUND_BOTTOM.redF(), BACKGROUND_BOTTOM.greenF(), BACKGROUND_BOTTOM.blueF()], dtype=np.float32)
    background = top * (1 - t) + bottom * t

    picture = np.clip((lit * coverage + background * (1 - coverage)) * 255.0, 0, 255).astype(np.uint8)
    return QPixmap.fromImage(images.array_to_image(picture))


def light_thumbnail(kind):
    """kind is 'Directional', 'Point', 'Spot' or 'Area'"""
    key = ("light", kind)
    if key not in _cache:
        _cache[key] = _LIGHTS[kind]()
    return _cache[key]


def _missing_thumbnail():
    pixmap, painter = _canvas()
    painter.setPen(QPen(QColor("#6a6a6a"), 8, Qt.SolidLine, Qt.RoundCap))
    painter.drawLine(88, 88, 168, 168)
    painter.drawLine(168, 88, 88, 168)
    painter.end()
    return pixmap
