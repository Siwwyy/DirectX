# DXI Project

## Overview

[Provide a brief overview of the DXI project, its purpose, and key features.]

## Projects and building `DXI` with CMake

### Repository layout (summary)

- `DXI/` – Primary application project. Contains `CMakeLists.txt` and the real application sources under `DXI/DXI/`. This is the only folder required to build the app.
- Other folders (examples, legacy projects, engine experiments) are present for reference (e.g. `DX_Projects/`, `MainApp/`, `Tutorial/`, `DXEngine/`). They are not required to build `DXI`.

### Prerequisites

- Windows with the Windows SDK installed.
- CMake 3.15 or newer.
- Visual Studio 2019/2022 (or another generator that supports C++14).  
  - For Visual Studio: use the matching generator (e.g. `"Visual Studio 17 2022" -A x64`).
- Optional: PIX for Windows (to enable PIX instrumentation).

### Build (recommended from repository root)

1. Create an out-of-source build directory and configure CMake:

    ```sh
    mkdir build
    cd build
    cmake -G "Visual Studio 17 2022" -A x64 ..
    ```

2. Build the Release configuration:

    ```sh
    cmake --build . --config Release
    ```

   This produces the executable `DXIApp` (if `WinMain.cpp` is present) in `build/bin/Release` (Debug builds go to `build/bin/Debug`).

### Alternative: Ninja or single-step configure

cmake -G "Ninja" -S DXI -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

### PIX support

This project supports PIX (WinPixEventRuntime) via three methods. Add one of the following when configuring/building `DXI`.

#### 1) Recommended: install with `vcpkg` (manifest mode)

1. Install and bootstrap `vcpkg` (see https://github.com/microsoft/vcpkg).
2. Ensure `DXI/vcpkg.json` includes `winpixeventruntime` (manifest file is provided).
3. Configure CMake using the vcpkg toolchain:

    ```sh
    cmake -S DXI -B build -G "Visual Studio 17 2022" -A x64 \
      -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
    cmake --build build --config Release
    ```

   This is the most reproducible approach for local and CI builds.

#### 2) Let CMake download the WinPixEventRuntime zip (convenience, Windows only)

CMake can download and extract the official `WinPixEventRuntime.zip` during configuration. This is intended for convenience only (not recommended for CI):

cmake -S DXI -B build -G "Visual Studio 17 2022" -A x64 -DDOWNLOAD_PIX=ON
cmake --build build --config Release

The archive is extracted into `build/third_party/WinPixEventRuntime` and CMake will attempt to use `Include/pix.h` and the library under `Lib/x64`.

#### 3) Install Microsoft.Pix from winget (Windows)

When PIX cannot be obtained via `vcpkg` or the CMake downloader, the `DXI/CMakeLists.txt` can attempt to install PIX via `winget`. By default, CMake will try winget if `-DUSE_WINGET=ON` and `USE_PIX=1`.

To let CMake attempt winget install during configuration:

cmake -S DXI -B build -G "Visual Studio 17 2022" -A x64 -DUSE_PIX=1 -DUSE_WINGET=1

CMake invokes:

winget install --id Microsoft.Pix -e --accept-package-agreements --accept-source-agreements
or
winget install microsoft.pix --accept-package-agreements

The flags `--accept-package-agreements` and `--accept-source-agreements` cause winget to auto-accept license/source prompts so the install won't block interactively. **IMPORTANT:** review and accept the publisher license terms yourself before enabling automatic install.

### Notes and troubleshooting

- If winget reports "found matching input criteria" or other ambiguity, run the exact command manually in an elevated shell to inspect the interactive output and resolve which package id to use.
- For CI and reproducible builds, prefer `vcpkg` (manifest mode) instead of winget or ad-hoc downloads.
- To disable PIX attempts entirely, pass `-DUSE_PIX=OFF` to CMake.

### Troubleshooting

- If CMake cannot find `pix.h` or the PIX runtime library, prefer the `vcpkg` installation method.
- Ensure you build for the same architecture as the installed PIX runtime (x64 vs x86).
- Automatic download is Windows-only and requires PowerShell available on PATH.

### Useful CMake options

- Use `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake` to enable `vcpkg` for dependencies.
- To force a particular generator or architecture, pass the appropriate `-G` and `-A` arguments to `cmake`.

### Troubleshooting

- If CMake errors about multiple entry points, ensure only a single `WinMain.cpp` exists under `DXI/DXI/`.
- If headers are not found, verify the `DXI` include directories (`DXI/DXI/`, `DXI/DXI/Objects`, `DXI/DXI/Geometry`, etc.) are present and that you used an out-of-source build.

## PIX / WinPixEventRuntime

This project can automatically obtain Microsoft PIX's `WinPixEventRuntime` at configure time when PIX support is enabled.

### How it works

- CMake prefers system-installed PIX (found via `pix.h` and `WinPixEventRuntime.lib`).
- If not found and `-DUSE_PIX=ON`, CMake will attempt to use `nuget.exe` to install the `WinPixEventRuntime` NuGet package into the build folder.
- If `nuget.exe` is not available on PATH, CMake will download a local copy to `build/tools/nuget/nuget.exe` and use it.
- The NuGet package is installed into `build/nuget_packages/`. CMake searches that tree for `pix.h` and a `.lib` and applies the include/link paths to the `DXIApp` target.

### Configuration options

- `-DUSE_PIX=ON|OFF` — enable or disable PIX support (default: `ON`).
- `-DWINPIX_VERSION=<version>` — optional: pin a specific NuGet package version (leave empty to install latest).

### Common commands

- Configure and build (PIX enabled, latest):

  cmake -B build -S . -DUSE_PIX=ON
  cmake --build build --config Release

- Configure and build with a pinned WinPix version:

  cmake -B build -S . -DUSE_PIX=ON -DWINPIX_VERSION=1.0.240308001

### Locations created by the bootstrap

- `build/tools/nuget/nuget.exe` — local nuget.exe (if not present on PATH).
- `build/nuget_packages/` — NuGet package files for `WinPixEventRuntime`.

### Troubleshooting

- If automatic install fails, you can install via vcpkg and pass `-DCMAKE_TOOLCHAIN_FILE` (recommended for reproducible builds):

  vcpkg install winpixeventruntime:x64-windows

- Alternatively, install Microsoft.Pix via winget / Microsoft Store and ensure `pix.h` and `WinPixEventRuntime.lib` are visible to CMake.
- The CMake script reports the discovered include and library paths during configuration.

### Notes

- This automatic bootstrap is Windows-only.
- The CMakeLists currently sets `CMAKE_CXX_STANDARD` to 17. If you need C++14 for other parts of your workflow, adjust the CMake settings accordingly before configuring.

---

[Add any additional sections or information relevant to the project, such as usage instructions, contribution guidelines, or license information.]

This revised `README.md` maintains the original structure while clearly documenting how to build the `DXI` application with CMake and enabling PIX support. It ensures that the information is organized logically and is easy to follow for users who want to build the project. The new section on PIX installation options has been seamlessly integrated into the existing document structure.

This updated version enhances clarity and organization, ensuring that users can easily navigate the document and find the information they need.