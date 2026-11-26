"""A picture of a window as the compositor shows it, the engine's viewport included.

Qt's own ways of grabbing do not see the viewport. widget.grab() paints the widget, and the engine's
swap chain is not painted by Qt. Grabbing a window by its id reads the window's own surface, which the
swap chain never writes to. Asking the compositor for the window with PW_RENDERFULLCONTENT is what
returns what is on screen, and it also works when the window is covered by others.
"""

import ctypes
from ctypes import wintypes

from PySide6.QtGui import QImage

PW_RENDERFULLCONTENT = 2
BI_RGB = 0
DIB_RGB_COLORS = 0

_user32 = ctypes.WinDLL("user32")
_gdi32 = ctypes.WinDLL("gdi32")

_user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
_user32.GetDC.restype = wintypes.HDC
_user32.GetDC.argtypes = [wintypes.HWND]
_user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
_user32.PrintWindow.argtypes = [wintypes.HWND, wintypes.HDC, wintypes.UINT]
_gdi32.CreateCompatibleDC.restype = wintypes.HDC
_gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
_gdi32.CreateCompatibleBitmap.restype = wintypes.HBITMAP
_gdi32.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
_gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
_gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
_gdi32.DeleteDC.argtypes = [wintypes.HDC]
_gdi32.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                             ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]


class _BitmapInfoHeader(ctypes.Structure):
    _fields_ = [("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG), ("biHeight", wintypes.LONG),
                ("biPlanes", wintypes.WORD), ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
                ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG),
                ("biYPelsPerMeter", wintypes.LONG), ("biClrUsed", wintypes.DWORD),
                ("biClrImportant", wintypes.DWORD)]


def grab_window(hwnd):
    """The whole window, frame and all, as a QImage. Null when Windows refuses."""
    rect = wintypes.RECT()
    _user32.GetWindowRect(hwnd, ctypes.byref(rect))
    width, height = rect.right - rect.left, rect.bottom - rect.top
    if width <= 0 or height <= 0:
        return QImage()

    screen_dc = _user32.GetDC(None)
    memory_dc = _gdi32.CreateCompatibleDC(screen_dc)
    bitmap = _gdi32.CreateCompatibleBitmap(screen_dc, width, height)
    previous = _gdi32.SelectObject(memory_dc, bitmap)

    try:
        if not _user32.PrintWindow(hwnd, memory_dc, PW_RENDERFULLCONTENT):
            return QImage()

        # Negative height asks for rows top to bottom, the order QImage wants
        header = _BitmapInfoHeader(ctypes.sizeof(_BitmapInfoHeader), width, -height, 1, 32, BI_RGB, 0, 0, 0, 0, 0)
        pixels = ctypes.create_string_buffer(width * height * 4)
        _gdi32.GetDIBits(memory_dc, bitmap, 0, height, pixels, ctypes.byref(header), DIB_RGB_COLORS)

        # The buffer is gone when this returns, so the image takes its own copy
        return QImage(pixels, width, height, width * 4, QImage.Format_RGB32).copy()
    finally:
        _gdi32.SelectObject(memory_dc, previous)
        _gdi32.DeleteObject(bitmap)
        _gdi32.DeleteDC(memory_dc)
        _user32.ReleaseDC(None, screen_dc)
