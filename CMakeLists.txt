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

if(WIN32)
    set(ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
endif()

add_executable(pesto_gb ${SOURCES} ${ICON_RESOURCE})
target_link_libraries(pesto_gb PRIVATE SDL2::SDL2main SDL2::SDL2-static)