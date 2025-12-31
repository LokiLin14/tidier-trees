cmake_minimum_required(VERSION 3.10)
project(tidier_trees C)

set(CMAKE_C_STANDARD 11)

# Find SDL2
find_package(SDL2 REQUIRED)

# Use pkg-config for SDL2_ttf and SDL2_image since they may not have CMake configs
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Add your source files
add_executable(tidier_trees
        src/main.c
        src/trees.c
        src/utils.c
        src/utils.h
)
target_link_libraries(tidier_trees
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        m
)
target_include_directories(tidier_trees PRIVATE
        src
        ${SDL2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/external/clay
)

# Testing
enable_testing()

# Find CMocka
find_package(cmocka REQUIRED)

# Enable AddressSanitizer for tests
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")

add_executable(test_tidier_trees
        src/utils.c
        src/utils.h
        test/main.c
)
target_link_libraries(test_tidier_trees cmocka)
target_include_directories(test_tidier_trees PRIVATE src)

add_test(NAME test_tidier_trees COMMAND test_tidier_trees)