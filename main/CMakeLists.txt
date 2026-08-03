file(GLOB_RECURSE COMPONENT_SRCS
    "*.c"
)

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS
        "."
    REQUIRES
        esp32-camera
        esp_lcd_ili9341
        esp_lcd_touch_xpt2046
        esp-dsp
        esp_timer
        esp_lcd
        esp_event
        spiffs
)
