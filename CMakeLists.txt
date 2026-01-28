if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "Vá para '/Laranjal-Foods/build' e use o comando 'cmake ..'\n"
        "ou\n"
        "Vá para '/Laranjal-Foods/' e use o comando 'cmake -S . -B build'"
)
endif()

cmake_minimum_required(VERSION 3.28.3)

project(laranjal_foods VERSION 0.1)

set(SOURCES
    src/main.cpp
    src/init_and_destroy.cpp
    src/build_strings.cpp
)

add_compile_options(-Wall)

add_executable(${PROJECT_NAME} ${SOURCES})