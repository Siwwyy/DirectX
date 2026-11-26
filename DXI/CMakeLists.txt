cmake_minimum_required(VERSION 3.26)
project(DXIApp LANGUAGES CXX)


# INCLUDE
include(FetchContent)

##

set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            OFF)
set(WINPIX_VERSION                  "" CACHE STRING "Optional WinPixEventRuntime version to install via nuget (leave empty for latest)")

option(USE_VCPKG    "Enable vcpkg toolchain (pass -DCMAKE_TOOLCHAIN_FILE=...)" OFF)
option(USE_PIX      "Try to enable PIX support if available" ON)

# Source directory containing the DXI project (DXI/DXI)
set(DXI_SRC_DIR                     "${CMAKE_CURRENT_SOURCE_DIR}/DXI")
set(DXI_THIRD_PARTY_DIRECTORY       "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Gather implementation sources only
file(GLOB_RECURSE DXI_SOURCES CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*.cpp"
    "${DXI_SRC_DIR}/*.c"
    "${DXI_SRC_DIR}/*.cc"
    "${DXI_SRC_DIR}/*.cxx"
)

# Superseded code, kept for reference but not built. Compiling it costs build time and
# keeps dead dependencies alive, so it is pulled out of the lists below and added back to
# the target as IDE-only. That also keeps the folder off the include path, so nothing live
# can quietly start including it again.
file(GLOB_RECURSE DXI_DEPRECATED CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/Deprecated/*.cpp"
    "${DXI_SRC_DIR}/Deprecated/*.h"
)

if(DXI_DEPRECATED)
    list(REMOVE_ITEM DXI_SOURCES ${DXI_DEPRECATED})
    set_source_files_properties(${DXI_DEPRECATED} PROPERTIES HEADER_FILE_ONLY TRUE)
    list(LENGTH DXI_DEPRECATED DXI_DEPRECATED_COUNT)
    message(STATUS "Deprecated files excluded from the build: ${DXI_DEPRECATED_COUNT}")
endif()

if(NOT DXI_SOURCES)
    message(FATAL_ERROR "No source files found under ${DXI_SRC_DIR}")
endif()

# Gather headers so IDEs show them in projects
file(GLOB_RECURSE DXI_HEADERS CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*.h"
    "${DXI_SRC_DIR}/*.hpp"
)

file(GLOB_RECURSE REMOVE_HEADERS "${DXI_SRC_DIR}/libs/*.h")

# remove DXC headers from the list of IDE-visible headers (they are included via target_include_directories)
# Deprecated headers go too, so that folder never reaches DXI_PUBLIC_INCLUDES below
list(REMOVE_ITEM DXI_HEADERS 
    ${REMOVE_HEADERS} # files to remove
    ${DXI_DEPRECATED}
)

if(NOT DXI_HEADERS)
    message(FATAL_ERROR "No header files found under ${DXI_HEADERS}")
endif()

# Detect WinMain sources and ensure at most one entry point
file(GLOB_RECURSE DXI_WINMAINS CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*WinMain.cpp"
    "${DXI_SRC_DIR}/*winmain.cpp"
)

# SHADERS or KERNELS
# Collect shader/kernel files. CONFIGURE_DEPENDS so a new .hlsl shows up in the solution
# on the next build, the same way sources and headers do.
file(GLOB_RECURSE DXI_SHADERS CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/shaders/*.glsl"
    "${DXI_SRC_DIR}/shaders/*.hlsl"
    "${DXI_SRC_DIR}/shaders/*.comp"
    "${DXI_SRC_DIR}/shaders/*.hip"
    "${DXI_SRC_DIR}/shaders/*.cl"
)

# Mark shader files as "IDE only" so generators (Visual Studio) won't attempt to compile them
if(DXI_SHADERS)
    set_source_files_properties(${DXI_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Mirror the on-disk folders in the .sln rather than dropping everything into the
# generator's flat "Source Files" / "Header Files" filters. One tree over sources, headers
# and shaders together, so Objects/Cube.cpp and Objects/Cube.h sit side by side under an
# Objects filter, Core/ holds its four pairs, and so on. Root level files stay at the root.
source_group(TREE "${DXI_SRC_DIR}" FILES ${DXI_SOURCES} ${DXI_HEADERS} ${DXI_SHADERS} ${DXI_DEPRECATED})

if(DXI_WINMAINS)
    list(LENGTH DXI_WINMAINS DXI_WINMAIN_COUNT)
    if(DXI_WINMAIN_COUNT GREATER 1)
        message(FATAL_ERROR "Multiple WinMain sources found under ${DXI_SRC_DIR}; please keep a single entry point.")
    endif()
endif()

# The engine is a library and the executable is a thin shell around it.
#
# DXICore is everything that renders: the device, the passes, the scene, the picking and the
# gizmo. It does not create a window or run a message loop, whoever hosts it hands over a window
# handle, see D3D12Renderer::SetWindowHandle. That is what lets something other than DXIApp host
# it, an editor with its own windows or a Python extension, with no change to the engine.
#
# DXIApp is what is left: WinMain and Win32Proc, which create the window and pump its messages.
set(DXI_SHELL_SOURCES
    "${DXI_SRC_DIR}/App/Win32Proc.cpp"
    ${DXI_WINMAINS}
)

set(DXI_CORE_SOURCES ${DXI_SOURCES})
list(REMOVE_ITEM DXI_CORE_SOURCES ${DXI_SHELL_SOURCES})

if(NOT DXI_CORE_SOURCES)
    message(FATAL_ERROR "No engine sources left for DXICore after taking out the shell")
endif()

# Create the library from all DXI sources (include headers for IDE).
# DXI_DEPRECATED is in the list only so the files stay browsable in the solution; they
# carry HEADER_FILE_ONLY so the generator will not try to compile them.
add_library(DXICore STATIC ${DXI_CORE_SOURCES} ${DXI_HEADERS} ${DXI_SHADERS} ${DXI_DEPRECATED})

add_executable(DXIApp WIN32 ${DXI_SHELL_SOURCES})
target_link_libraries(DXIApp PRIVATE DXICore)

# Opening the solution should start the application, not ALL_BUILD or the library
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT DXIApp)

# Include the DXI root plus every subfolder that actually holds a header.
# Derived from DXI_HEADERS rather than hand listed, so adding a new folder such as
# Core or LightTypes never needs an edit here. libs is left out on purpose, the DXC
# headers there are stale and would shadow the ones FetchContent downloads below.
set(DXI_PUBLIC_INCLUDES
    "${DXI_SRC_DIR}"
)

foreach(_header IN LISTS DXI_HEADERS)
    get_filename_component(_header_dir "${_header}" DIRECTORY)
    list(APPEND DXI_PUBLIC_INCLUDES "${_header_dir}")
endforeach()

list(REMOVE_DUPLICATES DXI_PUBLIC_INCLUDES)

# cgltf | glTF 2.0 parser, a single header. Pinned, since the loader is written against
# its accessor helpers and those have shifted across releases.
set(CGLTF_VERSION_TAG   "v1.15")
set(CGLTF_RELEASE_URL   "https://github.com/jkuhlmann/cgltf/archive/refs/tags/${CGLTF_VERSION_TAG}.zip")

message(STATUS "FetchContent: requesting cgltf from ${CGLTF_RELEASE_URL}")
FetchContent_Declare(cgltf URL ${CGLTF_RELEASE_URL})
FetchContent_MakeAvailable(cgltf)
message(STATUS "cgltf extracted to: ${cgltf_SOURCE_DIR}")

# Header only, so there is nothing to compile or link. One translation unit defines
# CGLTF_IMPLEMENTATION, see Geometry/GltfLoader.cpp.
#
# Include directories, definitions and libraries below are PUBLIC on DXICore: the shell and any
# other host include the engine's headers, which pull these in, and have to see them the same way.
target_include_directories(DXICore PUBLIC "${cgltf_SOURCE_DIR}")

# Dear ImGui | Debug panel. Fetched as a zip like DXC below, so no git is needed.
# Pinned on purpose, the DX12 backend changed its Init signature across releases.
set(IMGUI_VERSION_TAG   "v1.90.9")
set(IMGUI_RELEASE_URL   "https://github.com/ocornut/imgui/archive/refs/tags/${IMGUI_VERSION_TAG}.zip")

message(STATUS "FetchContent: requesting Dear ImGui from ${IMGUI_RELEASE_URL}")
FetchContent_Declare(imgui URL ${IMGUI_RELEASE_URL})
FetchContent_MakeAvailable(imgui)
message(STATUS "Dear ImGui extracted to: ${imgui_SOURCE_DIR}")

# ImGui ships as plain sources, there is no CMake target to link against
set(IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp"
)

target_sources(DXICore PRIVATE ${IMGUI_SOURCES})

# Out of the way in its own filter. These live under the build tree, so they are not part
# of the DXI_SRC_DIR tree above and would otherwise land in the default flat filter.
source_group("ThirdParty\\ImGui" FILES ${IMGUI_SOURCES})

target_include_directories(DXICore
    PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
)

# Add include directories to the target
target_include_directories(DXICore
    PUBLIC
        ${DXI_PUBLIC_INCLUDES}
)

# Compile settings
target_compile_definitions(DXICore PUBLIC UNICODE _UNICODE NOMINMAX) # work with wstring etc.
target_compile_features(DXICore PUBLIC cxx_std_14)

# Link system DirectX libraries on Windows
if(WIN32)
    target_link_libraries(DXICore
        PUBLIC
            d3d12
            dxgi
            d3dcompiler
    )

    if(USE_PIX)
        # 1) Prefer system/vcpkg-provided PIX
        find_path(PIX_INCLUDE_DIR NAMES pix.h HINTS ENV INCLUDE PATHS)
        find_library(PIX_RUNTIME_LIB NAMES WinPixEventRuntime WinPixEventRuntime.lib HINTS ENV LIB PATHS)

        if(PIX_INCLUDE_DIR AND PIX_RUNTIME_LIB)
            message(STATUS "PIX found: include=${PIX_INCLUDE_DIR}, lib=${PIX_RUNTIME_LIB}")
        else()
            message(STATUS "PIX not found on system include/library paths. Attempting NuGet-based install...")

            # Attempt to find nuget.exe on PATH; if missing, download a local copy into build/tools/nuget/nuget.exe
            find_program(NUGET_EXE NAMES nuget.exe nuget)
            set(_NUGET_LOCAL_DIR "${CMAKE_BINARY_DIR}/tools/nuget")
            set(_NUGET_LOCAL_EXE "${_NUGET_LOCAL_DIR}/nuget.exe")

            if(NOT NUGET_EXE)
                file(MAKE_DIRECTORY "${_NUGET_LOCAL_DIR}")
                if(NOT EXISTS "${_NUGET_LOCAL_EXE}")
                    message(STATUS "nuget.exe not found on PATH � downloading local copy to ${_NUGET_LOCAL_EXE}")
                    file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${_NUGET_LOCAL_EXE}" SHOW_PROGRESS STATUS _nuget_dl_status)
                    list(GET _nuget_dl_status 0 _nuget_dl_code)
                    if(NOT _nuget_dl_code EQUAL 0)
                        message(WARNING "Failed to download nuget.exe (exit ${_nuget_dl_code}). Manual installation of nuget.exe recommended.")
                    endif()
                endif()
                if(EXISTS "${_NUGET_LOCAL_EXE}")
                    set(NUGET_EXE "${_NUGET_LOCAL_EXE}")
                endif()
            endif()

            if(NUGET_EXE)
                message(STATUS "Using nuget executable: ${NUGET_EXE}")
                set(_NUGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/nuget_packages")
                file(MAKE_DIRECTORY "${_NUGET_OUTPUT_DIR}")

                if(WINPIX_VERSION)
                    set(_NUGET_VER_ARG -Version ${WINPIX_VERSION})
                else()
                    set(_NUGET_VER_ARG)
                endif()

                execute_process(
                    COMMAND "${NUGET_EXE}" install WinPixEventRuntime ${_NUGET_VER_ARG} -OutputDirectory "${_NUGET_OUTPUT_DIR}"
                    RESULT_VARIABLE _nuget_res
                    OUTPUT_VARIABLE _nuget_out
                    ERROR_VARIABLE _nuget_err
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE
                )

                if(_nuget_res EQUAL 0)
                    message(STATUS "nuget install completed; searching package tree for pix.h and .lib...")

                    # headers
                    file(GLOB_RECURSE _nuget_pix_headers CONFIGURE_DEPENDS "${_NUGET_OUTPUT_DIR}/*/Include/pix3.h")
                    if(_nuget_pix_headers)
                        list(GET _nuget_pix_headers 0 _first_pix)
                        get_filename_component(PIX_INCLUDE_DIR "${_first_pix}" DIRECTORY)
                        message(STATUS "Found pix.h via nuget in ${PIX_INCLUDE_DIR}")
                    endif()
                    # libs
                    file(GLOB_RECURSE _nuget_pix_libs CONFIGURE_DEPENDS "${_NUGET_OUTPUT_DIR}/*/WinPixEventRuntime.lib")
                    message(STATUS "NUGET PIX LIBS: ${_nuget_pix_libs}")
                    if(_nuget_pix_libs)
                        #message(STATUS "Found candidate PIX libs via nuget: ${_nuget_pix_libs}")
                        foreach(_lib IN LISTS _nuget_pix_libs)
                            string(TOLOWER "${_lib}" _lib_lc)
                            if(_lib_lc MATCHES "/x64/|\\\\x64\\\\|/X64/|\\\\X64\\\\")
                                set(_picked_pix_lib "${_lib}")
                                break()
                            endif()
                        endforeach()
                        #list(GET _nuget_pix_libs 0 _first_lib)
                        get_filename_component(PIX_RUNTIME_LIB "${_picked_pix_lib}" ABSOLUTE)
                        message(STATUS "Found PIX lib via nuget: ${PIX_RUNTIME_LIB}")
                    endif()

                    # dlls
                    file(GLOB_RECURSE _nuget_pix_dlls CONFIGURE_DEPENDS "${_NUGET_OUTPUT_DIR}/*/WinPixEventRuntime.dll")
                    message(STATUS "NUGET PIX DLLS: ${_nuget_pix_dlls}")
                    if(_nuget_pix_dlls)
                        #message(STATUS "Found candidate PIX DLLs via nuget: ${_nuget_pix_dlls}")
                        foreach(_dll IN LISTS _nuget_pix_dlls)
                            string(TOLOWER "${_dll}" _dll_lc)
                            if(_dll_lc MATCHES "/x64/|\\\\x64\\\\|/X64/|\\\\X64\\\\")
                                set(_picked_pix_dll "${_dll}")
                                break()
                            endif()
                        endforeach()
                        #list(GET _nuget_pix_dlls 0 _first_dll)
                        get_filename_component(PIX_RUNTIME_DLL "${_picked_pix_dll}" ABSOLUTE)
                        message(STATUS "Found PIX DLL via nuget: ${PIX_RUNTIME_DLL}")
                    endif()

                else()
                    message(WARNING "nuget install failed (exit ${_nuget_res}). Output: ${_nuget_out} ${_nuget_err}")
                endif()
            else()
                message(STATUS "nuget.exe unavailable; skipping NuGet installation.")
            endif()
        endif()

        # Apply found include/lib if available
        if(PIX_INCLUDE_DIR AND PIX_RUNTIME_LIB)
            target_include_directories(DXICore PUBLIC ${PIX_INCLUDE_DIR})
            target_link_libraries(DXICore PUBLIC ${PIX_RUNTIME_LIB})
            target_compile_definitions(DXICore PUBLIC USE_PIX=1)

            # Copy the runtime DLL(s) to the executable's folder after build. The runtime DLLs
            # belong next to whatever gets run, so this stays on DXIApp and not on the library.
            add_custom_command(TARGET DXIApp POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${PIX_RUNTIME_DLL}
                    $<TARGET_FILE_DIR:DXIApp>
                COMMENT "Copying WinPixEventRuntime.dll to executable folder"
            )
            message(STATUS "Will copy WinPixEventRuntime.dll from ${PIX_RUNTIME_DLL} to executable folder.")
            message(STATUS "PIX support enabled.")
        else()
            message(WARNING "PIX is unavailable. Options:\n - Install via vcpkg: `vcpkg install winpixeventruntime:x64-windows` and pass -DCMAKE_TOOLCHAIN_FILE.\n - Install Microsoft.Pix manually (winget or Microsoft Store) and make pix.h / WinPixEventRuntime.lib visible to CMake.\n - Disable PIX support with -DUSE_PIX=OFF.")
        endif()
    endif() # USE_PIX
endif() # WIN32

# DXI Compiler 
set(DXI_COMPILER_VERSION_TAG        "v1.9.2602.24")
set(DXI_COMPILER_ZIP_FILE_NAME      "dxc_2026_05_27.zip")
set(DXI_COMPILER_RELEASE_URL        "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXI_COMPILER_VERSION_TAG}/${DXI_COMPILER_ZIP_FILE_NAME}")

if(DXI_COMPILER_RELEASE_URL)

    # Actual fetching
    message(STATUS "FetchContent: requesting DXC from ${DXI_COMPILER_RELEASE_URL}")
    FetchContent_Declare(dxc_release URL ${DXI_COMPILER_RELEASE_URL})
    FetchContent_GetProperties(dxc_release)
    FetchContent_MakeAvailable(dxc_release) # downloads and extracts into dxc_release_SOURCE_DIR

    # release source dir (where files has been fetched)
    set(_DXC_DIR "${dxc_release_SOURCE_DIR}")
    message(STATUS "DXC extracted to: ${_DXC_DIR}")

    # Find dxc import lib (prefer dxcapi.lib) and runtime DLL (dxcompiler.dll)
    file(GLOB_RECURSE DXC_LIBS CONFIGURE_DEPENDS "${_DXC_DIR}/lib/x64/*.lib")
    file(GLOB_RECURSE DXC_DLLS CONFIGURE_DEPENDS "${_DXC_DIR}/bin/x64/*.dll")

    # add more verbosity for debugging
    message(STATUS "DXC libs: ${DXC_LIBS}")
    message(STATUS "DXC DLLs: ${DXC_DLLS}")

    # Link statically againts DXC import lib and include DXC headers
    target_include_directories(DXICore PUBLIC "${_DXC_DIR}/inc")
    target_link_libraries(DXICore PUBLIC ${DXC_LIBS})

else()

    set(DXC_LIBS "")
    set(DXC_DLL "")
    message(WARNING "DXC release URL not specified; skipping DXC fetch.")

endif()

# Link the DXC import DLLs to the target (just copy where .exe will be located of project)
if(DXC_DLLS)
    foreach(_dll IN LISTS DXC_DLLS)
        add_custom_command(TARGET DXIApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_dll}"
                "$<TARGET_FILE_DIR:DXIApp>"
            COMMENT "Copying ${_dll} to executable folder"
        )
    endforeach()
    message(STATUS "Will copy DXC runtime DLLs: ${DXC_DLLS}")
else()
    message(WARNING "dxc import lib found but dxcompiler.dll not located. Place dxcompiler.dll next to the EXE or add to PATH.")
endif()


# Per-configuration output directories. The library's archive goes under lib/, away from
# the executable and the DLLs next to it.
set_target_properties(DXICore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
)

set_target_properties(DXIApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# optionally link dxguid if available (resolves some IID_* / GUID link errors)
find_library(DXGUID_LIB NAMES dxguid dxguid.lib PATHS ENV LIB PATHS NO_DEFAULT_PATH)
if(DXGUID_LIB)
    target_link_libraries(DXICore PUBLIC ${DXGUID_LIB})
else()
    # linking plain "dxguid" also works with MSVC in many cases:
    target_link_libraries(DXICore PUBLIC dxguid)
endif()
# Python module | `import dxi`. The engine in a Python process, drawing into a window the caller
# owns, which is how an editor built with PySide6 hosts it. See python/bindings/dxi_module.cpp.
#
# The binding source is outside DXI/ on purpose: that tree is globbed into DXICore, and the
# engine must not know Python exists. Off, nothing below runs and nothing needs Python installed.
# Which Python it builds against is whatever find_package finds, pass -DPython_EXECUTABLE=... to
# pick one: the module only loads in the interpreter version it was built for.
option(DXI_BUILD_PYTHON "Build the dxi Python extension module" ON)

if(DXI_BUILD_PYTHON)
    find_package(Python 3.9 QUIET COMPONENTS Interpreter Development.Module)

    if(Python_FOUND)
        # Header only. Pinned, the binding is written against this release's enum and optional casters.
        set(PYBIND11_VERSION_TAG    "v2.13.6")
        set(PYBIND11_RELEASE_URL    "https://github.com/pybind/pybind11/archive/refs/tags/${PYBIND11_VERSION_TAG}.zip")
        set(PYBIND11_FINDPYTHON     ON) # use the Python found above, not its own search

        message(STATUS "FetchContent: requesting pybind11 from ${PYBIND11_RELEASE_URL}")
        FetchContent_Declare(pybind11 URL ${PYBIND11_RELEASE_URL})
        FetchContent_MakeAvailable(pybind11)

        file(GLOB_RECURSE DXI_PYTHON_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/python/bindings/*.cpp")

        pybind11_add_module(DXIPython MODULE ${DXI_PYTHON_SOURCES})
        target_link_libraries(DXIPython PRIVATE DXICore)

        # The file has to be called dxi.pyd for `import dxi`. The target is not, a target named
        # dxi would sit beside DXIApp and DXICore as a third project nobody can tell apart.
        set_target_properties(DXIPython PROPERTIES OUTPUT_NAME dxi)

        # Next to the executable, where the DLLs it needs are: Windows looks for a module's
        # dependencies in its own folder. Building DXIApp is what copies them there, so it comes first.
        add_dependencies(DXIPython DXIApp)
        set_target_properties(DXIPython PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG      "${CMAKE_BINARY_DIR}/bin/Debug"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE    "${CMAKE_BINARY_DIR}/bin/Release"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG      "${CMAKE_BINARY_DIR}/bin/Debug"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE    "${CMAKE_BINARY_DIR}/bin/Release"
        )
        message(STATUS "Python module dxi: ${Python_EXECUTABLE} (${Python_VERSION})")
    else()
        message(STATUS "Python with development files not found, the dxi module is not built. Pass -DDXI_BUILD_PYTHON=OFF to silence this.")
    endif()
endif()