cmake_minimum_required(VERSION 3.12)

add_library(D3D12
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12HelloWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/D3D12HelloWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DXSample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/DXSample.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/DXSampleHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Win32Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Win32Application.h
)

target_link_libraries(D3D12 PUBLIC Miscellaneous)
target_include_directories(D3D12 PUBLIC	"${CMAKE_CURRENT_SOURCE_DIR}/include") # PUBLIC -> consumers of the lib + lib