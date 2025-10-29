project(emulator)

add_library(emulator STATIC
    source/riscv/core.cpp
)
target_include_directories(emulator PUBLIC include)