cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pesto_gb_esp32)

add_subdirectory(${CMAKE_SOURCE_DIR}/../core core)