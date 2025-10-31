project(emulator)

add_library(emulator STATIC
    source/address_space.cpp
    source/riscv/core.cpp
)
target_include_directories(emulator PUBLIC include)
target_compile_options(emulator PRIVATE -Wall -Wextra -Wpedantic -Werror)