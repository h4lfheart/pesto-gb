cmake_minimum_required(VERSION 4.0)
project(pesto_gb)

set(CMAKE_CXX_STANDARD 23)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST OFF)
add_subdirectory(external/SDL)

file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/*.h
)

add_executable(pesto_gb ${SOURCES})
target_link_libraries(pesto_gb PRIVATE SDL2::SDL2main SDL2::SDL2-static)

