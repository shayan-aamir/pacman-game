cmake_minimum_required(VERSION 3.1)

project(Pacman)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Raylib REQUIRED)

add_executable(Pacman main.cpp)

target_link_libraries(Pacman PRIVATE -lraylib)
