cmake_minimum_required(VERSION 3.12)

add_library(Miscellaneous
            ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h
            ${CMAKE_CURRENT_SOURCE_DIR}/d3dx12.h
)

target_link_libraries(Miscellaneous
                      dxgi.lib
                      d3d12.lib
                      d3dcompiler.lib)

target_include_directories(Miscellaneous INTERFACE "${PROJECT_SOURCE_DIR}/Miscellaneous")