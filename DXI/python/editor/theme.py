"""The editor's look: a dark theme in the manner of Unreal Engine's editor.

Fusion draws the widgets and a dark palette colours them, so spin boxes, combo boxes and scroll bars keep their
native parts, arrows and all. The style sheet only does what a palette cannot: the dock titles, the menus, the
tab bars and the panels' own surfaces.
"""

from PySide6.QtGui import QColor, QFont, QPalette
from PySide6.QtWidgets import QApplication

# Darkest to lightest. Names say where they are used, not what they look like.
CHROME      = "#121212"  # menu bar, dock titles, tab bar, status bar
WINDOW      = "#1b1b1b"  # behind everything
PANEL       = "#202020"  # inside a dock
FIELD       = "#0f0f0f"  # anything typed in or listed
SECTION     = "#2a2a2a"  # a collapsible section's header
BORDER      = "#303030"
HOVER       = "#2d2d2d"
ACCENT      = "#2a7de1"  # focus, the active tab
SELECTION   = "#1e4f8f"  # selected row, checked tool
TEXT        = "#c8c8c8"
TEXT_DIM    = "#7a7a7a"
WARNING     = "#e0a030"

STYLE_SHEET = f"""
* {{ font-family: "Segoe UI"; font-size: 9pt; }}

QMainWindow, QDialog {{ background: {WINDOW}; }}
QWidget#Panel {{ background: {PANEL}; }}
QToolTip {{ background: {CHROME}; color: {TEXT}; border: 1px solid {BORDER}; padding: 3px; }}

QMenuBar {{ background: {CHROME}; border-bottom: 1px solid #000; padding: 1px 4px; }}
QMenuBar::item {{ background: transparent; padding: 4px 10px; border-radius: 3px; }}
QMenuBar::item:selected {{ background: {HOVER}; }}
QMenu {{ background: {WINDOW}; border: 1px solid {BORDER}; padding: 4px 0; }}
QMenu::item {{ padding: 5px 28px 5px 24px; }}
QMenu::item:selected {{ background: {SELECTION}; }}
QMenu::item:disabled {{ color: {TEXT_DIM}; }}
QMenu::separator {{ height: 1px; background: {BORDER}; margin: 4px 0; }}

QStatusBar {{ background: {CHROME}; border-top: 1px solid #000; color: {TEXT_DIM}; }}
QStatusBar::item {{ border: 0; }}

QDockWidget {{ titlebar-close-icon: none; titlebar-normal-icon: none; }}
QDockWidget::title {{
    background: {CHROME}; padding: 6px 10px; text-align: left;
    border-top: 2px solid {ACCENT}; border-bottom: 1px solid #000;
}}
QMainWindow::separator {{ background: #000; width: 2px; height: 2px; }}
QMainWindow::separator:hover {{ background: {ACCENT}; }}

QTabBar {{ background: {CHROME}; }}
QTabBar::tab {{
    background: {CHROME}; color: {TEXT_DIM}; padding: 6px 14px;
    border-top: 2px solid transparent; border-right: 1px solid #000;
}}
QTabBar::tab:selected {{ background: {PANEL}; color: {TEXT}; border-top: 2px solid {ACCENT}; }}
QTabBar::tab:hover:!selected {{ background: {HOVER}; }}

QScrollArea {{ border: 0; background: {PANEL}; }}
QScrollArea > QWidget > QWidget {{ background: {PANEL}; }}

QListWidget {{
    background: {FIELD}; alternate-background-color: #141414; border: 1px solid #000; outline: 0;
}}
QListWidget::item {{ padding: 3px 6px; border: 0; }}
QListWidget::item:hover {{ background: {HOVER}; }}
QListWidget::item:selected {{ background: {SELECTION}; color: #fff; }}

QToolButton#SectionHeader {{
    background: {SECTION}; border: 0; border-bottom: 1px solid #111; border-top: 1px solid #111;
    text-align: left; padding: 6px 8px; font-weight: 600;
}}
QToolButton#SectionHeader:hover {{ background: #323232; }}

QFrame#ViewportHeader {{ background: {CHROME}; border-bottom: 1px solid #000; }}
QFrame#ViewportHeader QToolButton, QFrame#ViewportHeader QLabel {{ padding: 3px 9px; }}
QFrame#ViewportHeader QToolButton {{ background: transparent; border: 1px solid transparent; border-radius: 3px; }}
QFrame#ViewportHeader QToolButton:hover {{ background: {HOVER}; }}
QFrame#ViewportHeader QToolButton:checked {{ background: {SELECTION}; border: 1px solid {ACCENT}; color: #fff; }}
QFrame#ViewportHeader QToolButton::menu-indicator {{ image: none; }}

QLabel#DetailsTitle {{ font-size: 11pt; font-weight: 600; padding: 8px 10px 6px 10px; }}
QLabel#Dim {{ color: {TEXT_DIM}; }}
QLabel#Warning {{ color: {WARNING}; }}

QPushButton {{ padding: 4px 12px; }}
"""


def apply(app: QApplication):
    app.setStyle("Fusion")
    app.setFont(QFont("Segoe UI", 9))

    palette = QPalette()
    roles = {
        QPalette.Window:          WINDOW,
        QPalette.WindowText:      TEXT,
        QPalette.Base:            FIELD,
        QPalette.AlternateBase:   "#141414",
        QPalette.ToolTipBase:     CHROME,
        QPalette.ToolTipText:     TEXT,
        QPalette.Text:            TEXT,
        QPalette.Button:          "#2c2c2c",
        QPalette.ButtonText:      TEXT,
        QPalette.BrightText:      "#ffffff",
        QPalette.Link:            ACCENT,
        QPalette.Highlight:       SELECTION,
        QPalette.HighlightedText: "#ffffff",
        QPalette.PlaceholderText: TEXT_DIM,
        QPalette.Light:           "#3a3a3a",
        QPalette.Midlight:        "#303030",
        QPalette.Mid:             "#1a1a1a",
        QPalette.Dark:            "#0a0a0a",
        QPalette.Shadow:          "#000000",
    }
    for role, color in roles.items():
        palette.setColor(role, QColor(color))

    # What a disabled widget shows, which in Fusion is otherwise too close to an enabled one
    for role in (QPalette.WindowText, QPalette.Text, QPalette.ButtonText):
        palette.setColor(QPalette.Disabled, role, QColor(TEXT_DIM))

    app.setPalette(palette)
    app.setStyleSheet(STYLE_SHEET)
