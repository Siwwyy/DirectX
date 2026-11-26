"""Finds and imports the engine module.

The module is a build product, dxi.<abi tag>.pyd in build/bin/<Config> next to the DLLs it needs
(dxcompiler, dxil, WinPixEventRuntime). It only loads into the Python version it was built for, which
is whichever one CMake found, see the DXIPython target.
"""

import importlib
import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def find_bin_dir(config=None):
    """The folder holding the module.

    DXI_BIN_DIR wins when set. Otherwise the requested configuration under build/bin, and with none
    requested the one whose module was built last, so a Release build after a Debug one is what runs.
    """
    override = os.environ.get("DXI_BIN_DIR")
    if override:
        return Path(override)

    bin_root = REPO_ROOT / "build" / "bin"
    configs = [config] if config else ["Debug", "Release"]

    found = []
    for name in configs:
        for module in (bin_root / name).glob("dxi*.pyd"):
            found.append((module.stat().st_mtime, module.parent))

    if not found:
        raise FileNotFoundError(
            f"No dxi module under {bin_root}. Build it with: "
            "cmake --build build --config Debug --target DXIPython "
            "(the Python version CMake found has to be the one running this).")

    return max(found, key=lambda entry: entry[0])[1]


def load(config=None):
    """Imports and returns the dxi module."""
    bin_dir = find_bin_dir(config)

    # The module's dependencies are looked up beside it already, this covers a host that moved it
    os.add_dll_directory(str(bin_dir))
    if str(bin_dir) not in sys.path:
        sys.path.insert(0, str(bin_dir))

    return importlib.import_module("dxi")
