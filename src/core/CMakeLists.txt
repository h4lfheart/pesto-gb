file(GLOB_RECURSE SOURCES *.cpp *.h)

add_library(pesto_core STATIC ${SOURCES})
target_include_directories(pesto_core PUBLIC .)