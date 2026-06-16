cmake_minimum_required(VERSION 3.15)
project(DXIApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_VCPKG "Enable vcpkg toolchain (pass -DCMAKE_TOOLCHAIN_FILE=...)" OFF)
option(USE_PIX "Try to enable PIX support if available" ON)
set(WINPIX_VERSION "" CACHE STRING "Optional WinPixEventRuntime version to install via nuget (leave empty for latest)")

# Source directory containing the DXI project (DXI/DXI)
set(DXI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DXI")

# Gather implementation sources only
file(GLOB_RECURSE DXI_SOURCES CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*.cpp"
    "${DXI_SRC_DIR}/*.c"
    "${DXI_SRC_DIR}/*.cc"
    "${DXI_SRC_DIR}/*.cxx"
)

if(NOT DXI_SOURCES)
    message(FATAL_ERROR "No source files found under ${DXI_SRC_DIR}")
endif()

# Gather headers so IDEs show them in projects
file(GLOB_RECURSE DXI_HEADERS CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*.h"
    "${DXI_SRC_DIR}/*.hpp"
)

# Detect WinMain sources and ensure at most one entry point
file(GLOB_RECURSE DXI_WINMAINS CONFIGURE_DEPENDS
    "${DXI_SRC_DIR}/*WinMain.cpp"
    "${DXI_SRC_DIR}/*winmain.cpp"
)

if(DXI_WINMAINS)
    list(LENGTH DXI_WINMAINS DXI_WINMAIN_COUNT)
    if(DXI_WINMAIN_COUNT GREATER 1)
        message(FATAL_ERROR "Multiple WinMain sources found under ${DXI_SRC_DIR}; please keep a single entry point.")
    endif()
endif()

# Create executable from all DXI sources (include headers for IDE)
add_executable(DXIApp WIN32 ${DXI_SOURCES} ${DXI_HEADERS})

# Include DXI root and common subfolders (only add if they exist)
set(DXI_PUBLIC_INCLUDES
    "${DXI_SRC_DIR}"
)

set(DXI_OPTIONAL_SUBDIRS
    "Objects"
    "Geometry"
    "libs"
    "Window"
)

foreach(_sub ${DXI_OPTIONAL_SUBDIRS})
    set(_full "${DXI_SRC_DIR}/${_sub}")
    if(EXISTS "${_full}")
        list(APPEND DXI_PUBLIC_INCLUDES "${_full}")
    endif()
endforeach()

target_include_directories(DXIApp
    PRIVATE
        ${DXI_PUBLIC_INCLUDES}
)

# Compile settings
target_compile_definitions(DXIApp PRIVATE UNICODE _UNICODE NOMINMAX) # work with wstring etc.
target_compile_features(DXIApp PRIVATE cxx_std_17)

# Link system DirectX libraries on Windows
if(WIN32)
    target_link_libraries(DXIApp
        PRIVATE
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
                    message(STATUS "nuget.exe not found on PATH — downloading local copy to ${_NUGET_LOCAL_EXE}")
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
                    file(GLOB_RECURSE _nuget_pix_headers CONFIGURE_DEPENDS "${_NUGET_OUTPUT_DIR}/*/pix3.h" "${_NUGET_OUTPUT_DIR}/*/Include/pix3.h")
                    if(_nuget_pix_headers)
                        list(GET _nuget_pix_headers 0 _first_pix)
                        get_filename_component(PIX_INCLUDE_DIR "${_first_pix}" DIRECTORY)
                        message(STATUS "Found pix.h via nuget in ${PIX_INCLUDE_DIR}")
                    endif()

                    file(GLOB_RECURSE _nuget_pix_libs CONFIGURE_DEPENDS "${_NUGET_OUTPUT_DIR}/x64/WinPixEventRuntime.lib" "${_NUGET_OUTPUT_DIR}/*.lib" "${_NUGET_OUTPUT_DIR}/x64/*.lib")
                    if(_nuget_pix_libs)
                        #message(STATUS "Found candidate PIX libs via nuget: ${_nuget_pix_libs}")
                        list(GET _nuget_pix_libs 2 _first_lib)
                        get_filename_component(PIX_RUNTIME_LIB "${_first_lib}" ABSOLUTE)
                        message(STATUS "Found PIX lib via nuget: ${PIX_RUNTIME_LIB}")
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
            target_include_directories(DXIApp PRIVATE ${PIX_INCLUDE_DIR})
            target_link_libraries(DXIApp PRIVATE ${PIX_RUNTIME_LIB})
            target_compile_definitions(DXIApp PRIVATE USE_PIX=1)
            message(STATUS "PIX support enabled.")
        else()
            message(WARNING "PIX is unavailable. Options:\n - Install via vcpkg: `vcpkg install winpixeventruntime:x64-windows` and pass -DCMAKE_TOOLCHAIN_FILE.\n - Install Microsoft.Pix manually (winget or Microsoft Store) and make pix.h / WinPixEventRuntime.lib visible to CMake.\n - Disable PIX support with -DUSE_PIX=OFF.")
        endif()
    endif() # USE_PIX
endif() # WIN32

# Try to locate dxc import lib (dxcapi.lib). Add more HINTS if you place the files elsewhere.
find_library(DXC_IMPORT_LIB
    NAMES dxcapi dxcapi.lib dxcompiler.lib
    HINTS
        "${CMAKE_BINARY_DIR}/third_party/dxc/lib/x64"
        "${CMAKE_SOURCE_DIR}/third_party/dxc/lib/x64"
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib"
        "$ENV{ProgramFiles}/DirectXShaderCompiler/lib"
    PATH_SUFFIXES lib
)
message(STATUS "${CMAKE_BINARY_DIR}/third_party/dxc/lib/x64")

if(DXC_IMPORT_LIB)
    message(STATUS "Found dxc import lib: ${DXC_IMPORT_LIB}")
    target_link_libraries(DXIApp PRIVATE "${DXC_IMPORT_LIB}")

    # Try to find the matching dxcompiler.dll near the import lib
    get_filename_component(_dxc_lib_dir "${CMAKE_SOURCE_DIR}/third_party/dxc/bin/x64" DIRECTORY)

    set(_cand1 "${_dxc_lib_dir}/dxcompiler.dll")
    set(_cand2 "${_dxc_lib_dir}/x64/dxcompiler.dll")
    set(DXC_DLL "")
    
    if(EXISTS "${_cand1}")
        set(DXC_DLL "${_cand1}")
        message(STATUS "Found dxc dll import lib directory: ${_cand1}")
    elseif(EXISTS "${_cand2}")
        set(DXC_DLL "${_cand2}")
        message(STATUS "Found dxc dll import lib directory: ${_cand2}")
    endif()

    if(DXC_DLL)
        # Copy the runtime DLL to the target output folder after build
        add_custom_command(TARGET DXIApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXC_DLL}"
                "$<TARGET_FILE_DIR:DXIApp>"
            COMMENT "Copying dxcompiler.dll to executable folder"
        )
        message(STATUS "Will copy dxcompiler.dll from ${DXC_DLL} to executable folder.")
    else()
        message(WARNING "dxc import lib found but dxcompiler.dll not located. Place dxcompiler.dll next to the EXE or add to PATH.")
    endif()
else()
    message(WARNING "dxc import lib (dxcapi.lib) not found. Install DirectXShaderCompiler and place dxcapi.lib/dxcompiler.dll in a known folder, or install via vcpkg.")
endif()

# Per-configuration output directories
set_target_properties(DXIApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)