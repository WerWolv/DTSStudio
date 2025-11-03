cmake_minimum_required(VERSION 3.20)
project(impl)

add_subdirectory(interface)
add_subdirectory(emulator)

add_library(impl STATIC
    $<TARGET_OBJECTS:interface>
    $<TARGET_OBJECTS:emulator>
)
install(TARGETS impl DESTINATION "${CMAKE_INSTALL_PREFIX}")