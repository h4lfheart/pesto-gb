set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/SDL external/SDL)

file(GLOB_RECURSE SOURCES *.cpp *.h)

if(WIN32)
    set(ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
endif()

add_executable(pesto_gb_desktop ${SOURCES} ${ICON_RESOURCE})
add_compile_options(-O3)
target_include_directories(pesto_gb_desktop PRIVATE ${CMAKE_SOURCE_DIR}/external/argparse/include)
target_link_libraries(pesto_gb_desktop PRIVATE pesto_core SDL2::SDL2main SDL2::SDL2-static)