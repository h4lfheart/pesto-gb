FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
        REQUIRES esp_lcd_ili9341 esp_lcd driver spiffs)

target_link_libraries(${COMPONENT_LIB} PRIVATE pesto_gb_core)

target_compile_options(${COMPONENT_LIB} PRIVATE
        -std=gnu++23
)