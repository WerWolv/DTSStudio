cmake_minimum_required(VERSION 3.20)
project(interface)

set(CMAKE_CXX_STANDARD 26)

add_library(interface STATIC
    source/interface.cpp
)
target_include_directories(interface PUBLIC include)
target_compile_options(interface PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/resources)
target_link_libraries(interface PRIVATE emulator)
