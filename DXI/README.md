# DXI

Deferred Direct3D 12 renderer, written to learn the API.

A frame is `ShadowPass` (depth per light into a shared atlas), `BasePass` (scene into a
G-buffer), `LightPass` (shades it with a full screen triangle), then the ImGui panel.

- Directional, point and area lights, 4 at once (`MAX_LIGHTS`)
- Shadow atlas, 2048 per slice and 512 per point light cube face
- Six material slots: diffuse, normal, specular, metalness, roughness, AO. Mips on upload
- Primitives: cube, plane, UV sphere with adjustable density
- Scenes: a built in test set, Sponza, and the Cornell Box
- Loaders: Wavefront OBJ, glTF 2.0
- Constant buffers defined once and shared by C++ and HLSL, `DXI/shaders/SharedTypes.h`
- `BASELINE` switches, one per pass, that swap the stock lighting or shadowing for stubs
  to write your own against. Set in `LightPass.cpp` and `ShadowPass.cpp`
- PIX markers per pass and per object
- Debug panel spawns primitives and lights from assets found on disk

Covers the `DXI` project only. Sibling folders in the repository are older experiments.

## Quick start

```sh
setup.bat            # download assets, configure into build\
build.bat            # Debug
build.bat Release
```

Run `build\bin\Debug\DXIApp.exe`. `setup.bat` calls `download_assets.bat`, and `build.bat`
configures first if there is no build tree, so either script alone works on a fresh clone.

## Scenes

```sh
DXIApp.exe                     # built in: cubes, ground, sphere, Spot
DXIApp.exe -scene sponza       # Sponza alone, at the origin
DXIApp.exe -scene cornell      # Cornell Box, one ceiling light
DXIApp.exe -sponza -cornell    # shorthands for the two above
DXIApp.exe -scene both         # built in and Sponza, Sponza 40 units along +Z
```

In `both`, Sponza is offset clear of the ground plane, and the sun's shadow box stays sized
to the built in scene, so Sponza goes unshadowed. Use `-scene sponza` for shadows on it.

The Cornell Box is built to the measurements published at
[graphics.cornell.edu](https://www.graphics.cornell.edu/online/box/data.html), divided by 100
and mirrored on X for our left handed space. **No colour bleeding**: the famous red and green
cast onto the white walls is indirect light, and this renderer only does direct.

The scene is fixed at startup; there is no runtime teardown path.

## Command line

`-name`, `--name` or `/name`, case insensitive. Values as `-width 1920` or `-width=1920`.
Output goes to the debug output, not a console, so attach a debugger or use DebugView.

| Switch | Effect |
| --- | --- |
| `-scene default\|sponza\|both\|cornell` | Which scene to build. Default `default`. |
| `-sponza`, `-cornell` | Shorthand for the matching `-scene`. |
| `-gpuvalidation` | GPU based validation. Commonly 10x slower. |
| `-nodebuglayer` | Skip the debug layer. Needed without Graphics Tools. |
| `-nobreak` | Do not break into the debugger on debug layer messages. |
| `-warp` | WARP software adapter, to tell a driver bug from an engine bug. |
| `-vsync` | Sync interval 1. Off by default. |
| `-width N`, `-height N` | Window size in pixels. |
| `-help`, `-?` | This list. |

## Assets

Textures are committed, meshes are not: the Visual Studio `.gitignore` `*.obj` rule swallows
Wavefront meshes along with MSVC object files.

```sh
download_assets.bat            # whatever is missing
download_assets.bat --force    # re-fetch
```

Needs `curl.exe` and `tar.exe`, both in System32 on Windows 10 1803 and later. Everything
lands in `DXI/models/<name>/`. A missing mesh is logged and skipped, not fatal. Sponza is 71
files and 50 MB fetched one at a time, because Khronos ships no per model download.

| Asset | Source | Licence |
| --- | --- | --- |
| Spot | [Keenan Crane's model repository](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/) | CC0 1.0 |
| Sponza | [Khronos glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Sponza) | CC-BY 3.0 |
| Surface materials | [Poly Haven](https://polyhaven.com/) | CC0 |

Assets are discovered on disk, not listed in code. A content folder holding
`<folder>_diff_1k.png` is a material; a `DXI/models` folder holding an `.obj` is a mesh.
Both show up in the panel's spawn pickers.

## Layout

```
DXI/
  CMakeLists.txt        globs sources, headers and shaders
  setup.bat             configure
  build.bat             build
  download_assets.bat   fetch meshes
  DXI/
    WinMain.cpp, pch.h
    App/                D3D12Renderer, Win32Proc
    Core/               device, swap chain, depth buffer, frame context,
                        shader compiler, PSO, root signature builder
    Geometry/           vertex layouts, OBJ and glTF loaders
    Lights/             directional, point, area
    Material/           material, descriptor heap, texture upload, asset catalog
    Math/               transforms, constant buffer size aliases
    Objects/            camera, primitive, cube, plane, sphere, model
    Passes/             shadow, base, light
    RenderTargets/      G-buffer, shadow atlas, cube map
    UI/                 ImGui debug panel
    Utility/            helpers, paths, command line, PIX events, shader registers
    shaders/            HLSL, plus SharedTypes.h. Compiled at startup, so these
                        sit in the project for browsing only
    Deprecated/         older forward renderer, kept for reference, not compiled
    models/             downloaded meshes, not in the repository
    sky/                panorama for image based lighting, currently unused
    <material folders>  committed textures, one per Poly Haven asset
```

`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` picks up any new `.cpp`, `.h` or `.hlsl` under
`DXI/`, so nothing is listed by hand. The globs re-run after MSBuild has loaded the project,
so **the first build after adding or deleting a file always fails and the second passes**.
`build.bat` retries once for this. See `.cursor/rules/cmake-file-registration.mdc`.

## Dependencies

Fetched at configure time into `build/_deps`. Nothing to install, no git needed.

| Dependency | Version | Purpose |
| --- | --- | --- |
| [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler) | v1.9.2602.24 | Compiles the HLSL at startup |
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.90.9 | Debug panel |
| [cgltf](https://github.com/jkuhlmann/cgltf) | v1.15 | glTF 2.0 parsing |

Needs Windows with the Windows SDK, CMake 3.26 or newer (the scripts will find the copy
Visual Studio ships), and a C++17 compiler. PIX is optional, see below.

## Build by hand

The batch files are thin wrappers around:

```sh
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Debug          # or Release
```

Output is `build/bin/<config>/DXIApp.exe`. Ninja works with
`-G Ninja -DCMAKE_BUILD_TYPE=Release`. `setup.bat` forwards extra arguments, so
`setup.bat -DUSE_PIX=OFF` works.

## PIX

Markers compile to nothing when `WinPixEventRuntime` is absent, so the build never depends
on it. See `DXI/Utility/PixEvents.h`.

| Option | Effect |
| --- | --- |
| `-DUSE_PIX=ON\|OFF` | Enable or disable the search. Default `ON`. |
| `-DWINPIX_VERSION=<version>` | Pin a NuGet version. Empty takes the latest. |
| `-DUSE_VCPKG=ON` | Expect PIX from a vcpkg toolchain. Default `OFF`. |

CMake tries a system install first (`pix.h` plus `WinPixEventRuntime.lib`, which covers
vcpkg), then the NuGet package into `build/nuget_packages/`, downloading `nuget.exe` to
`build/tools/nuget/` if it is not on PATH. Failing both is a warning, not an error. The
runtime DLL is copied next to the executable.

For CI, prefer vcpkg. There is no manifest here, so install into the triplet and pass the
toolchain:

```sh
vcpkg install winpixeventruntime:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

PIX itself, for capturing, is separate: `winget install --id Microsoft.Pix -e`.

## Troubleshooting

**Build fails right after adding or deleting a file.** Expected, build again. If a second
build still fails, run `setup.bat`. `C1083: Cannot open source file` is the same thing.

**Shaders fail to compile.** They are compiled at startup, so an HLSL error stops the app
launching rather than failing the build. `dxcompiler.dll` and `dxil.dll` must sit next to
the executable; CMake copies them and warns if it cannot find them.

**DXGI error about session 0.** Swap chains cannot be created from a non-interactive
session. Run from a normal desktop session.

**CMake warns PIX is unavailable.** Harmless. Use vcpkg to fix it, match the architecture,
or pass `-DUSE_PIX=OFF`.

**Textures or meshes missing.** `download_assets.bat --force`.

## Licence

`LICENSE` at the repository root. Dependencies and downloaded assets carry their own, above.
