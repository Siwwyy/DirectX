"""A preview of one mesh that can be turned around, for the editor tab of an object.

The mesh comes out of the engine as arrays (dxi.Renderer.get_mesh) and is drawn here, on the CPU, by a small
z-buffered rasterizer written in numpy. That keeps the preview independent of the swap chain the viewport owns, so it
works while the viewport is hidden behind another tab, and it needs no second D3D12 device.

The rasterizer follows the engine's conventions: left handed, Y up, row vectors, an object's rotation is
Rx * Ry * Rz of its Euler angles in degrees. What is on screen is therefore what the viewport shows.

Orbiting the camera is the default drag. With Rotate Model on, the same drag turns the model in the preview, and
objectRotated tells the tab the angles it now has. The preview never writes to the engine.
"""

import math

import numpy as np
from PySide6.QtCore import QPointF, QTimer, Qt, Signal
from PySide6.QtGui import QColor, QFont, QPainter, QPen
from PySide6.QtWidgets import QWidget

import images
from theme import TEXT_DIM
from viewport import CONTENT_MIME

FOV_DEGREES = 40.0
MODES = ("Lit", "Clay", "Normals", "Wireframe")

BACKGROUND_TOP = np.array([0.21, 0.21, 0.22], dtype=np.float32)
BACKGROUND_BOTTOM = np.array([0.09, 0.09, 0.10], dtype=np.float32)
CLAY = np.array([0.74, 0.74, 0.76], dtype=np.float32)
WIRE_COLOR = np.array([0.50, 0.75, 1.00], dtype=np.float32)

FAST_SCALE = 0.5            # share of the size the view is drawn at while it is being dragged
SETTLE_MS = 180             # without input for this long, the full quality picture is drawn
SUPERSAMPLE = 1.5           # the resting picture is drawn this much larger and scaled down, for smooth edges
SUPERSAMPLE_PIXELS = 900_000  # ... unless the view has more pixels than this
CHUNK_FRAGMENTS = 1_500_000   # candidate pixels a batch of triangles is tested against at once, bounds the memory
MAX_EDGES = 400_000


# Rotations. Row vectors, as DirectXMath has them: a point turns by being multiplied from the left.

def rotation_from_euler(degrees):
    """Rx * Ry * Rz of three angles in degrees, the engine's RotationFromEulerDegrees"""
    a, b, c = (math.radians(d) for d in degrees)
    ca, sa, cb, sb, cc, sc = math.cos(a), math.sin(a), math.cos(b), math.sin(b), math.cos(c), math.sin(c)
    rx = np.array([[1, 0, 0], [0, ca, sa], [0, -sa, ca]], dtype=np.float64)
    ry = np.array([[cb, 0, -sb], [0, 1, 0], [sb, 0, cb]], dtype=np.float64)
    rz = np.array([[cc, sc, 0], [-sc, cc, 0], [0, 0, 1]], dtype=np.float64)
    return rx @ ry @ rz


def euler_from_rotation(rotation):
    """The three angles in degrees that rotation_from_euler turns back into this matrix"""
    sb = -rotation[0, 2]
    b = math.asin(max(-1.0, min(1.0, sb)))
    if abs(sb) < 0.999999:
        a = math.atan2(rotation[1, 2], rotation[2, 2])
        c = math.atan2(rotation[0, 1], rotation[0, 0])
    else:
        # Looking straight along an axis, x and z turn the same way and only their sum matters
        a = math.atan2(-rotation[2, 1], rotation[1, 1])
        c = 0.0
    return tuple(math.degrees(v) for v in (a, b, c))


def rotation_about(axis, radians):
    """A turn about an axis as a row vector matrix. The angle follows the right hand rule on the numbers, which in
    the engine's left handed frame turns the other way on screen, so callers pick the sign that looks right."""
    axis = np.asarray(axis, dtype=np.float64)
    axis = axis / (np.linalg.norm(axis) or 1.0)
    c, s = math.cos(radians), math.sin(radians)
    x, y, z = axis
    cross = np.array([[0, -z, y], [z, 0, -x], [-y, x, 0]])
    column = c * np.eye(3) + s * cross + (1 - c) * np.outer(axis, axis)
    return column.T


def _normalize(v):
    return v / (np.linalg.norm(v) or 1.0)


class MeshData:
    """What get_mesh returns, as arrays. uvs and normals are None for a mesh that has none."""

    def __init__(self, positions, uvs, normals, indices):
        self.positions = np.frombuffer(positions, dtype=np.float32).reshape(-1, 3).astype(np.float64)
        count = len(self.positions)

        uvs = np.frombuffer(uvs, dtype=np.float32).reshape(-1, 2)
        normals = np.frombuffer(normals, dtype=np.float32).reshape(-1, 3)
        self.uvs = uvs.astype(np.float64) if len(uvs) == count else None
        self.normals = normals.astype(np.float64) if len(normals) == count else None

        indices = np.frombuffer(indices, dtype=np.uint32)
        indices = indices[:len(indices) // 3 * 3].reshape(-1, 3)
        self.triangles = indices[(indices < count).all(axis=1)].astype(np.int64)

        if count:
            self.centre = (self.positions.min(axis=0) + self.positions.max(axis=0)) / 2
        else:
            self.centre = np.zeros(3)
        self._edges = None

    @classmethod
    def from_engine(cls, renderer, index):
        return cls(*renderer.get_mesh(index))

    @property
    def edges(self):
        """Each edge once, as a pair of vertex numbers"""
        if self._edges is None:
            both = np.concatenate([self.triangles[:, [0, 1]], self.triangles[:, [1, 2]], self.triangles[:, [2, 0]]])
            both.sort(axis=1)
            key = np.unique(both[:, 0] * max(len(self.positions), 1) + both[:, 1])
            self._edges = np.stack([key // max(len(self.positions), 1), key % max(len(self.positions), 1)], axis=1)
        return self._edges


# Rasterizing

def rasterize(sx, sy, inv_z, triangles, width, height):
    """Draws triangles into a depth buffer. sx and sy are the corners in pixels, inv_z is 1 / view depth for each,
    which is what varies linearly across the screen, so the larger it is the nearer.

    Returns, for every pixel: inv_z of the nearest surface (0 for none), its triangle (-1 for none), and the two
    perspective correct barycentric weights of its second and third corner.
    """
    pixels = width * height
    depth = np.zeros(pixels, dtype=np.float32)
    owner = np.full(pixels, -1, dtype=np.int32)
    weight1 = np.zeros(pixels, dtype=np.float32)
    weight2 = np.zeros(pixels, dtype=np.float32)

    if len(triangles) == 0:
        return depth, owner, weight1, weight2

    i0, i1, i2 = triangles[:, 0], triangles[:, 1], triangles[:, 2]
    x0, y0, x1, y1, x2, y2 = sx[i0], sy[i0], sx[i1], sy[i1], sx[i2], sy[i2]
    z0, z1, z2 = inv_z[i0], inv_z[i1], inv_z[i2]

    area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)

    low_x = np.minimum(np.minimum(x0, x1), x2)
    high_x = np.maximum(np.maximum(x0, x1), x2)
    low_y = np.minimum(np.minimum(y0, y1), y2)
    high_y = np.maximum(np.maximum(y0, y1), y2)

    keep = (np.abs(area) > 1e-9) & (high_x >= 0) & (low_x <= width - 1) & (high_y >= 0) & (low_y <= height - 1)
    ids = np.nonzero(keep)[0]
    if len(ids) == 0:
        return depth, owner, weight1, weight2

    min_x = np.clip(np.floor(low_x[ids]), 0, width - 1).astype(np.int64)
    max_x = np.clip(np.ceil(high_x[ids]), 0, width - 1).astype(np.int64)
    min_y = np.clip(np.floor(low_y[ids]), 0, height - 1).astype(np.int64)
    max_y = np.clip(np.ceil(high_y[ids]), 0, height - 1).astype(np.int64)

    # Triangles are tested against a window of whole pixels around them, and all of one window size go together.
    # Sizes are powers of two, so a handful of batches covers a mesh whatever its triangles look like.
    bits_x = np.ceil(np.log2(np.maximum(max_x - min_x + 1, 1))).astype(np.int64)
    bits_y = np.ceil(np.log2(np.maximum(max_y - min_y + 1, 1))).astype(np.int64)
    group_key = bits_x * 32 + bits_y
    order = np.argsort(group_key, kind="stable")
    keys, starts = np.unique(group_key[order], return_index=True)
    ends = list(starts[1:]) + [len(order)]

    for key, start, end in zip(keys, starts, ends):
        win_w, win_h = 1 << int(key // 32), 1 << int(key % 32)
        per_batch = max(1, CHUNK_FRAGMENTS // (win_w * win_h))
        along_x = np.arange(win_w, dtype=np.int64)[None, None, :]
        along_y = np.arange(win_h, dtype=np.int64)[None, :, None]

        for first in range(start, end, per_batch):
            local = order[first:min(first + per_batch, end)]
            tri = ids[local]

            gx = min_x[local][:, None, None] + along_x
            gy = min_y[local][:, None, None] + along_y

            ex1, ey1 = (x1[tri] - x0[tri])[:, None, None], (y1[tri] - y0[tri])[:, None, None]
            ex2, ey2 = (x2[tri] - x0[tri])[:, None, None], (y2[tri] - y0[tri])[:, None, None]
            scale = (1.0 / area[tri])[:, None, None]

            dx = gx + 0.5 - x0[tri][:, None, None]
            dy = gy + 0.5 - y0[tri][:, None, None]
            b1 = (dx * ey2 - ex2 * dy) * scale
            b2 = (ex1 * dy - dx * ey1) * scale
            b0 = 1.0 - b1 - b2

            inside = (b0 >= 0) & (b1 >= 0) & (b2 >= 0) & (gx <= max_x[local][:, None, None]) & (gy <= max_y[local][:, None, None])
            batch, row, col = np.nonzero(inside)
            if len(batch) == 0:
                continue

            frag_tri = tri[batch]
            f0, f1, f2 = b0[batch, row, col], b1[batch, row, col], b2[batch, row, col]
            frag_inv = f0 * z0[frag_tri] + f1 * z1[frag_tri] + f2 * z2[frag_tri]
            frag_pixel = gy[batch, row, 0] * width + gx[batch, 0, col]

            # Nearest per pixel inside this batch, then against what earlier batches left
            nearest_first = np.argsort(-frag_inv, kind="stable")
            _, first_of_pixel = np.unique(frag_pixel[nearest_first], return_index=True)
            winners = nearest_first[first_of_pixel]
            pixel = frag_pixel[winners]
            wins = frag_inv[winners] > depth[pixel]
            winners, pixel = winners[wins], pixel[wins]

            depth[pixel] = frag_inv[winners]
            owner[pixel] = frag_tri[winners]
            weight1[pixel] = f1[winners] * z1[frag_tri[winners]] / frag_inv[winners]
            weight2[pixel] = f2[winners] * z2[frag_tri[winners]] / frag_inv[winners]

    return depth, owner, weight1, weight2


def draw_edges(image, depth, sx, sy, inv_z, edges, width, height, thick):
    """Lines along the edges, hidden where a surface is in front. image is (height * width, 3), changed in place."""
    if len(edges) > MAX_EDGES:
        edges = edges[::len(edges) // MAX_EDGES + 1]

    a, b = edges[:, 0], edges[:, 1]
    visible = (inv_z[a] > 0) & (inv_z[b] > 0)
    a, b = a[visible], b[visible]
    if len(a) == 0:
        return

    dx, dy = sx[b] - sx[a], sy[b] - sy[a]
    counts = np.ceil(np.maximum(np.abs(dx), np.abs(dy))).astype(np.int64) + 1
    # An edge crossing the screen from far outside is not worth its pixels
    counts = np.minimum(counts, 4 * (width + height))
    if counts.sum() > 6_000_000:
        keep = counts < 4 * (width + height) // 2
        a, b, dx, dy, counts = a[keep], b[keep], dx[keep], dy[keep], counts[keep]

    edge = np.repeat(np.arange(len(a)), counts)
    along = (np.arange(len(edge)) - np.repeat(np.cumsum(counts) - counts, counts)) / np.maximum(counts[edge] - 1, 1)

    x = sx[a][edge] + dx[edge] * along
    y = sy[a][edge] + dy[edge] * along
    z = inv_z[a][edge] + (inv_z[b] - inv_z[a])[edge] * along

    for step_x, step_y in ((0, 0), (1, 0), (0, 1), (1, 1)) if thick else ((0, 0),):
        px = np.floor(x).astype(np.int64) + step_x
        py = np.floor(y).astype(np.int64) + step_y
        ok = (px >= 0) & (px < width) & (py >= 0) & (py < height)
        pixel = py[ok] * width + px[ok]
        # In front of or on the surface, with a little slack for the depth the surface was drawn with
        shown = z[ok] >= depth[pixel] * 0.99
        image[pixel[shown]] = WIRE_COLOR


class View:
    """Where the camera is and what it looks at: the object's centre, from a distance, at two angles"""

    def __init__(self):
        self.yaw = 35.0
        self.pitch = 20.0
        self.distance = 3.0
        self.pan = np.zeros(3)

    def basis(self):
        """eye, right, up, forward, for a camera looking at the pan point"""
        yaw, pitch = math.radians(self.yaw), math.radians(self.pitch)
        offset = np.array([math.sin(yaw) * math.cos(pitch), math.sin(pitch), -math.cos(yaw) * math.cos(pitch)])
        eye = self.pan + offset * self.distance
        forward = _normalize(self.pan - eye)
        right = _normalize(np.cross([0.0, 1.0, 0.0], forward))
        up = np.cross(forward, right)
        return eye, right, up, forward


def render(mesh, positions, normals, view, size, mode, texture, near):
    """The mesh as an (height, width, 3) uint8 picture. positions and normals are the mesh as it stands in the
    preview's space: centred, scaled and turned."""
    width, height = size
    eye, right, up, forward = view.basis()

    relative = positions - eye
    depth_view = relative @ forward
    focal = 0.5 * height / math.tan(math.radians(FOV_DEGREES) / 2)

    in_front = depth_view > near
    safe = np.where(in_front, depth_view, 1.0)
    sx = width / 2 + focal * (relative @ right) / safe
    sy = height / 2 - focal * (relative @ up) / safe
    inv_z = np.where(in_front, 1.0 / safe, 0.0)

    # A triangle with a corner behind the camera is left out rather than clipped, which only shows when the view
    # is inside the mesh
    triangles = mesh.triangles[in_front[mesh.triangles].all(axis=1)]
    depth, owner, weight1, weight2 = rasterize(sx, sy, inv_z, triangles, width, height)

    ramp = np.linspace(0.0, 1.0, height, dtype=np.float32)[:, None, None]
    picture = np.broadcast_to(BACKGROUND_TOP * (1 - ramp) + BACKGROUND_BOTTOM * ramp, (height, width, 3)).reshape(-1, 3).copy()

    covered = np.nonzero(owner >= 0)[0]
    if len(covered):
        tri = triangles[owner[covered]]
        w1 = weight1[covered].astype(np.float64)[:, None]
        w2 = weight2[covered].astype(np.float64)[:, None]
        w0 = 1.0 - w1 - w2

        point = w0 * positions[tri[:, 0]] + w1 * positions[tri[:, 1]] + w2 * positions[tri[:, 2]]

        if normals is not None:
            normal = w0 * normals[tri[:, 0]] + w1 * normals[tri[:, 1]] + w2 * normals[tri[:, 2]]
            normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-12)
        else:
            normal = np.cross(positions[tri[:, 1]] - positions[tri[:, 0]], positions[tri[:, 2]] - positions[tri[:, 0]])
            normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-12)

        to_eye = eye - point
        to_eye /= np.maximum(np.linalg.norm(to_eye, axis=1, keepdims=True), 1e-12)
        # Seen from behind a surface is lit the way its front is, so a plane or an open mesh has no dark side
        normal = np.where((normal * to_eye).sum(axis=1, keepdims=True) < 0, -normal, normal)

        if mode == "Normals":
            colour = normal * 0.5 + 0.5
        else:
            if mode == "Lit" and texture is not None and mesh.uvs is not None:
                uv = w0 * mesh.uvs[tri[:, 0]] + w1 * mesh.uvs[tri[:, 1]] + w2 * mesh.uvs[tri[:, 2]]
                th, tw = texture.shape[:2]
                albedo = texture[(uv[:, 1] * th).astype(np.int64) % th, (uv[:, 0] * tw).astype(np.int64) % tw] / 255.0
            elif mode == "Wireframe":
                albedo = np.full((len(covered), 3), 0.30)
            else:
                albedo = np.broadcast_to(CLAY, (len(covered), 3))

            key_light = _normalize(-0.45 * right + 0.75 * up - 0.5 * forward)
            fill_light = _normalize(0.7 * right - 0.2 * up - 0.5 * forward)
            diffuse = 0.85 * np.clip(normal @ key_light, 0, 1) + 0.25 * np.clip(normal @ fill_light, 0, 1)
            ambient = 0.14 + 0.14 * (normal @ up * 0.5 + 0.5)
            half = key_light + to_eye
            half /= np.maximum(np.linalg.norm(half, axis=1, keepdims=True), 1e-12)
            specular = 0.30 * np.clip((normal * half).sum(axis=1), 0, 1) ** 48

            colour = albedo * (ambient + diffuse)[:, None] + specular[:, None]

        picture[covered] = np.clip(colour, 0.0, 1.0)

    if mode == "Wireframe":
        draw_edges(picture, depth, sx, sy, inv_z, mesh.edges, width, height, thick=width > 700)

    return (picture.reshape(height, width, 3) * 255.0 + 0.5).astype(np.uint8)


class MeshPreview(QWidget):
    objectRotated = Signal(float, float, float)  # the Euler angles the model now has, in degrees
    contentDropped = Signal(list)                # rows of content browser tiles let go on the preview

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(300, 240)
        self.setFocusPolicy(Qt.StrongFocus)
        self.setAcceptDrops(True)
        self.setCursor(Qt.OpenHandCursor)

        self._mesh = None
        self._euler = (0.0, 0.0, 0.0)
        self._scale = (1.0, 1.0, 1.0)
        self._texture = None
        self._mode = "Lit"
        self._rotate_object = False
        self._view = View()
        self._radius = 1.0
        self._posed = None     # (positions, normals) for the current rotation and scale
        self._pose_key = None

        self._moving = False
        self._drag = None      # (button, last position) while a button is down
        self._settle = QTimer(self)
        self._settle.setSingleShot(True)
        self._settle.setInterval(SETTLE_MS)
        self._settle.timeout.connect(self._on_settled)

        self._shown = None     # (key, picture) of the last frame, kept while nothing changed
        self._dim_hint = "Drag: orbit    Shift or middle drag: pan    Wheel: zoom    F: frame"

    # What is shown

    def set_mesh(self, mesh):
        self._mesh = mesh
        self._pose_key = None
        self._shown = None
        self._update_radius()
        self.frame()

    def set_transform(self, euler, scale):
        euler = tuple(float(v) for v in euler)
        scale = tuple(float(v) for v in scale)
        if euler == self._euler and scale == self._scale:
            return

        rescaled = scale != self._scale
        self._euler, self._scale = euler, scale
        if rescaled:
            self._update_radius()
        self.update()

    def set_texture(self, texture):
        """An (h, w, 3) uint8 array to paint the mesh with in Lit mode, or None for grey"""
        if texture is not self._texture:
            self._texture = texture
            self.update()

    def set_mode(self, mode):
        self._mode = mode
        self.update()

    def set_rotate_object(self, on):
        self._rotate_object = bool(on)
        self.setCursor(Qt.SizeAllCursor if on else Qt.OpenHandCursor)

    def frame(self):
        """Back to a view of the whole object"""
        self._view.pan = np.zeros(3)
        self._view.distance = self._radius / math.sin(math.radians(FOV_DEGREES) / 2) * 1.45
        self._touched()

    def _update_radius(self):
        if self._mesh is None or len(self._mesh.positions) == 0:
            self._radius = 1.0
            return
        scale = np.array(self._scale)
        self._radius = max(float(np.linalg.norm((self._mesh.positions - self._mesh.centre) * scale, axis=1).max()), 1e-4)

    def _pose(self):
        """The mesh turned and scaled the way the object is, around its own centre"""
        key = (self._euler, self._scale)
        if key != self._pose_key:
            scale = np.where(np.abs(self._scale) < 1e-6, 1e-6, self._scale)
            rotation = rotation_from_euler(self._euler)
            positions = ((self._mesh.positions - self._mesh.centre) * scale) @ rotation
            normals = None
            if self._mesh.normals is not None:
                # The inverse transpose of scale * rotation, which for a rotation is the inverse scale
                normals = (self._mesh.normals / scale) @ rotation
                normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
            self._posed = (positions, normals)
            self._pose_key = key
        return self._posed

    # Drawing

    def _touched(self):
        """The view is being changed: draw it cheaply now, and well once it has rested"""
        self._moving = True
        self._settle.start()
        self.update()

    def _on_settled(self):
        self._moving = False
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), QColor("#1b1b1b"))

        if self._mesh is None or len(self._mesh.triangles) == 0:
            painter.setPen(QColor(TEXT_DIM))
            painter.drawText(self.rect(), Qt.AlignCenter, "This object has no mesh to show")
            return

        ratio = self.devicePixelRatioF()
        full = (max(int(self.width() * ratio), 1), max(int(self.height() * ratio), 1))
        if self._moving:
            samples = FAST_SCALE
        else:
            samples = SUPERSAMPLE if full[0] * full[1] <= SUPERSAMPLE_PIXELS else 1.0
        size = (max(int(full[0] * samples), 1), max(int(full[1] * samples), 1))

        key = (size, self._mode, self._euler, self._scale, self._view.yaw, self._view.pitch, self._view.distance,
               tuple(self._view.pan), id(self._texture), id(self._mesh))
        if self._shown is None or self._shown[0] != key:
            positions, normals = self._pose()
            pixels = render(self._mesh, positions, normals, self._view, size, self._mode, self._texture, 0.02 * self._radius)
            self._shown = (key, images.array_to_image(pixels))

        painter.setRenderHint(QPainter.SmoothPixmapTransform)
        painter.drawImage(self.rect(), self._shown[1])
        self._paint_overlay(painter)

    def _paint_overlay(self, painter):
        painter.setRenderHint(QPainter.Antialiasing)
        font = QFont(painter.font())
        font.setPointSizeF(8.5)
        painter.setFont(font)

        painter.setPen(QColor(TEXT_DIM))
        mesh = self._mesh
        painter.drawText(10, 18, f"{len(mesh.positions):,} vertices   {len(mesh.triangles):,} triangles")
        painter.drawText(10, self.height() - 10, self._dim_hint)

        # The world's axes as the camera sees them, in the corner
        _, right, up, _ = self._view.basis()
        origin = QPointF(self.width() - 46, self.height() - 46)
        for axis, color, name in (((1, 0, 0), "#cc3d3d", "X"), ((0, 1, 0), "#4fa34f", "Y"), ((0, 0, 1), "#3d77cc", "Z")):
            tip = origin + QPointF(float(np.dot(axis, right)), -float(np.dot(axis, up))) * 30
            painter.setPen(QPen(QColor(color), 2))
            painter.drawLine(origin, tip)
            painter.setPen(QColor(color).lighter(140))
            painter.drawText(tip + QPointF(-3, 4) + (tip - origin) * 0.18, name)

    # Input

    def mousePressEvent(self, event):
        self.setFocus()
        self._drag = (event.button(), event.position())

    def mouseReleaseEvent(self, event):
        self._drag = None
        self.setCursor(Qt.SizeAllCursor if self._rotate_object else Qt.OpenHandCursor)

    def mouseMoveEvent(self, event):
        if self._drag is None or self._mesh is None:
            return

        button, last = self._drag
        position = event.position()
        dx, dy = position.x() - last.x(), position.y() - last.y()
        self._drag = (button, position)

        view = self._view
        if button == Qt.RightButton:
            view.distance = float(np.clip(view.distance * math.exp(dy * 0.01), 0.1 * self._radius, 60 * self._radius))
        elif button == Qt.MiddleButton or (button == Qt.LeftButton and event.modifiers() & Qt.ShiftModifier):
            _, right, up, _ = view.basis()
            per_pixel = 2 * view.distance * math.tan(math.radians(FOV_DEGREES) / 2) / max(self.height(), 1)
            view.pan = view.pan + (-right * dx + up * dy) * per_pixel
        elif button == Qt.LeftButton and self._rotate_object:
            self._turn_object(dx, dy)
        elif button == Qt.LeftButton:
            view.yaw -= dx * 0.4
            view.pitch = float(np.clip(view.pitch + dy * 0.4, -89.0, 89.0))
        self._touched()

    def _turn_object(self, dx, dy):
        """The object follows the cursor like a ball held at its near side: dragging right turns its front to the
        right, dragging down turns it down. Both turns are about the view's axes, not the object's own."""
        _, right, up, _ = self._view.basis()
        turn = rotation_about(up, -math.radians(dx * 0.5)) @ rotation_about(right, -math.radians(dy * 0.5))
        euler = euler_from_rotation(rotation_from_euler(self._euler) @ turn)

        self._euler = euler
        self.objectRotated.emit(*euler)

    def wheelEvent(self, event):
        steps = event.angleDelta().y() / 120.0
        self._view.distance = float(np.clip(self._view.distance * 0.9 ** steps, 0.1 * self._radius, 60 * self._radius))
        self._touched()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_F:
            self.frame()
        else:
            super().keyPressEvent(event)

    # Drops, of material tiles from the content browser

    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat(CONTENT_MIME):
            event.acceptProposedAction()

    def dragMoveEvent(self, event):
        if event.mimeData().hasFormat(CONTENT_MIME):
            event.acceptProposedAction()

    def dropEvent(self, event):
        if not event.mimeData().hasFormat(CONTENT_MIME):
            return
        rows = [int(part) for part in bytes(event.mimeData().data(CONTENT_MIME)).decode().split(",") if part]
        event.acceptProposedAction()
        self.contentDropped.emit(rows)
