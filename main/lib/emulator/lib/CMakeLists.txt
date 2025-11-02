cmake_minimum_required(VERSION 3.20)
project(emulator)

set(CMAKE_CXX_STANDARD 26)

add_library(emulator STATIC
    source/address_space.cpp
    source/riscv/core.cpp
    source/ffi.cpp
)
target_include_directories(emulator PUBLIC include)
target_compile_options(emulator PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/resources)

install(TARGETS emulator DESTINATION ".")