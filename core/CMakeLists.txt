file(GLOB_RECURSE SOURCES *.cpp *.h)

add_library(pesto_gb_core STATIC ${SOURCES})
target_include_directories(pesto_gb_core PUBLIC ..)