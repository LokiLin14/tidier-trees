cmake_minimum_required(VERSION 3.10)
project(tidier_trees C)

set(CMAKE_C_STANDARD 11)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Add your source files
add_executable(tidier_trees
        src/main.c
        src/trees.c
        src/utils.c
        src/utils.h
)
target_link_libraries(tidier_trees ${SDL2_LIBRARIES} m)
target_include_directories(tidier_trees PRIVATE src)

# Testing
enable_testing()

# Find CMocka
find_package(cmocka REQUIRED)

add_executable(test_tidier_trees
        src/utils.c
        src/utils.h
        test/main.c
)
target_link_libraries(test_tidier_trees cmocka)
target_include_directories(test_tidier_trees PRIVATE src)

add_test(NAME test_tidier_trees COMMAND test_tidier_trees)