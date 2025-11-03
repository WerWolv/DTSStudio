cmake_minimum_required(VERSION 3.20)
project(emulator)

set(CMAKE_CXX_STANDARD 26)

add_library(emulator STATIC
    source/address_space.cpp
    source/riscv/core.cpp
)
target_include_directories(emulator PUBLIC include)
