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
list(REMOVE_ITEM DXI_HEADERS 
    ${REMOVE_HEADERS} # files to remove
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
# Collect shader/kernel files
file(GLOB_RECURSE DXI_SHADERS
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

# Create a virtual folder in the .sln
source_group(TREE "${DXI_SRC_DIR}/shaders" PREFIX "Shaders" FILES ${DXI_SHADERS})

if(DXI_WINMAINS)
    list(LENGTH DXI_WINMAINS DXI_WINMAIN_COUNT)
    if(DXI_WINMAIN_COUNT GREATER 1)
        message(FATAL_ERROR "Multiple WinMain sources found under ${DXI_SRC_DIR}; please keep a single entry point.")
    endif()
endif()

# Create executable from all DXI sources (include headers for IDE)
add_executable(DXIApp WIN32 ${DXI_SOURCES} ${DXI_HEADERS} ${DXI_SHADERS})

# Include DXI root and common subfolders (only add if they exist)
set(DXI_PUBLIC_INCLUDES
    "${DXI_SRC_DIR}"
)

set(DXI_OPTIONAL_SUBDIRS
    "Geometry"
    "Objects"
    "Passes"
)

foreach(_sub ${DXI_OPTIONAL_SUBDIRS})
    set(_full "${DXI_SRC_DIR}/${_sub}")
    if(EXISTS "${_full}")
        list(APPEND DXI_PUBLIC_INCLUDES "${_full}")
    endif()
endforeach()

# Add include directories to the target
target_include_directories(DXIApp
    PRIVATE
        ${DXI_PUBLIC_INCLUDES}
)

# Compile settings
target_compile_definitions(DXIApp PRIVATE UNICODE _UNICODE NOMINMAX) # work with wstring etc.
target_compile_features(DXIApp PRIVATE cxx_std_14)

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
            target_include_directories(DXIApp PRIVATE ${PIX_INCLUDE_DIR})
            target_link_libraries(DXIApp PRIVATE ${PIX_RUNTIME_LIB})
            target_compile_definitions(DXIApp PRIVATE USE_PIX=1)

            # Copy the runtime DLL(s) to the target output folder after build
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
    target_include_directories(DXIApp PRIVATE "${_DXC_DIR}/inc")
    target_link_libraries(DXIApp PRIVATE ${DXC_LIBS})

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


# Per-configuration output directories
set_target_properties(DXIApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# optionally link dxguid if available (resolves some IID_* / GUID link errors)
find_library(DXGUID_LIB NAMES dxguid dxguid.lib PATHS ENV LIB PATHS NO_DEFAULT_PATH)
if(DXGUID_LIB)
    target_link_libraries(DXIApp PRIVATE ${DXGUID_LIB})
else()
    # linking plain "dxguid" also works with MSVC in many cases:
    target_link_libraries(DXIApp PRIVATE dxguid)
endif()