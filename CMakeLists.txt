cmake_minimum_required(VERSION 4.0)
project(pesto_gb)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(src/core)

if(ESP32)
    add_subdirectory(src/esp32)
else()
    add_subdirectory(src/desktop)
endif()