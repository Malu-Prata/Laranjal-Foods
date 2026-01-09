cmake_minimum_required(VERSION 3.28.3)

project(laranjal_foods VERSION 0.1)

set(SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})