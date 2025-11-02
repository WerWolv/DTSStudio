project(main)

add_executable(main
    source/main.cpp
)
target_include_directories(main PUBLIC include)
target_link_libraries(main PUBLIC emulator)
