cmake_minimum_required(VERSION 3.12)
project(MyApp VERSION 1.0.0)

add_executable(MyApp WIN32 Main.cpp)

target_link_libraries(MyApp PRIVATE D3D12)
