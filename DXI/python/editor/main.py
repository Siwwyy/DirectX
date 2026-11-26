"""DXI Editor, the engine inside a PySide6 window.

    python python/editor/main.py
    python python/editor/main.py --config Release --engine-args "-scene sponza -nobreak"

The engine is the dxi module, built by the DXIPython target, and runs in this process. PySide6 comes from
python/requirements.txt, install it into the Python that built the module (see dxi_loader.py).
"""

import argparse
import os
import sys

# The editor's modules sit beside this file and are imported by plain name
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QApplication

import capture
import dxi_loader
import theme
from main_window import MainWindow


def parse_arguments():
    parser = argparse.ArgumentParser(description="DXI Editor")
    parser.add_argument("--config", choices=["Debug", "Release"],
                        help="which build of the engine to load, the newest one when absent")
    parser.add_argument("--engine-args", default="",
                        help="switches for the engine as on its command line, for instance \"-scene sponza -nobreak\"")
    parser.add_argument("--screenshot", metavar="PATH",
                        help="save a picture of the window once it has rendered for a while, then quit")
    parser.add_argument("--screenshot-delay", type=float, default=4.0, metavar="SECONDS",
                        help="how long to run before the picture, the scene needs a few seconds to build")
    parser.add_argument("--select", type=int, default=-1, metavar="INDEX",
                        help="object to select at start, for the picture")
    return parser.parse_args()


def main():
    arguments = parse_arguments()
    dxi = dxi_loader.load(arguments.config)

    app = QApplication(sys.argv[:1])
    theme.apply(app)
    window = MainWindow(dxi, arguments.engine_args)
    window.show()

    if arguments.screenshot:
        def take_picture():
            if arguments.select >= 0 and window.viewport.has_engine:
                window.viewport.renderer.selected = arguments.select
                # Gives the selection a few frames to be drawn before the picture is taken
                QTimer.singleShot(600, save)
            else:
                save()

        def save():
            # The engine presents past Qt, so Qt's own grabs would show no viewport, see capture.py
            picture = capture.grab_window(int(window.winId()))
            if picture.isNull() or not picture.save(arguments.screenshot):
                print(f"Could not save a picture to {arguments.screenshot}", file=sys.stderr)

            # Closing, not quitting, so closeEvent shuts the engine down in order
            window.close()

        QTimer.singleShot(int(arguments.screenshot_delay * 1000), take_picture)

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
