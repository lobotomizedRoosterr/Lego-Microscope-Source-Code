#include "components/include/touch.h"
#include "components/include/spi_bus_manager.h"
#include "main/pin_config.h"
#include "components/include/display.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "TOUCH";
esp_lcd_touch_handle_t tp;

esp_err_t init_touch(void) {
    esp_err_t ret = ESP_OK;

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(T_CS_PIN);
    tp_io_config.pclk_hz=1000000;

    // Attach the TOUCH to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = DISPLAY_HEIGHT,
        .y_max = DISPLAY_WIDTH,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 1,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));

    return ret;
}
touch_data get_touch() {
        // Read touch data (call this periodically)
    esp_lcd_touch_read_data(tp);

    // Get touch coordinates
    uint16_t x[1];
    uint16_t y[1];
    uint16_t str[1];
    uint8_t cnt = 0;

    bool pr = esp_lcd_touch_get_coordinates(tp, x, y, str, &cnt, 1);

    touch_data ret = (touch_data) {
        .pressed=pr,
        .touch_strength=str[0],
        .touch_x=x[0],
        .touch_y=y[0]
    };
    return ret;
}