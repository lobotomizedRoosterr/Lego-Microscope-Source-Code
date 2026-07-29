#include "components/include/display.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "main/app_state.h"
#include "components/include/spi_bus_manager.h"
#include "main/pin_config.h"

#define LCD_SPI_HOST        SPI2_HOST
#define LCD_SPI_CLOCK_HZ    (40 * 1000 * 1000)   

#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LCD_COLOR_BITS      16
#define LCD_QUEUE_DEPTH     3

#define ILI9341_CMD_RDMADCTL 0x0B

esp_lcd_panel_handle_t lcd_panel_handle;

static const char *TAG = "DISPLAY";

esp_lcd_panel_io_handle_t lcd_io_handle = NULL;

//static uint16_t* frame_buffer;


uint32_t get_time_millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

esp_err_t init_display() {
    esp_err_t ret;

    //frame_buffer = heap_caps_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);

    gpio_config_t tcs_cfg = {
    .pin_bit_mask = (1ULL << T_CS_PIN),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
};
gpio_config(&tcs_cfg);
gpio_set_level(T_CS_PIN, 1);  // deasserted
    // ---- LCD panel IO / driver ----
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num         = LCD_CS_PIN,
        .dc_gpio_num         = LCD_DC_PIN,
        .spi_mode            = 0,
        .pclk_hz             = LCD_SPI_CLOCK_HZ,
        .trans_queue_depth   = LCD_QUEUE_DEPTH,
        .lcd_cmd_bits        = LCD_CMD_BITS,
        .lcd_param_bits      = LCD_PARAM_BITS,
    };
    

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
                                    &io_config, &lcd_io_handle);


    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_spi failed: %s", esp_err_to_name(ret));
        return ret;
    }
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST_PIN,
        .bits_per_pixel = LCD_COLOR_BITS,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian    = LCD_RGB_DATA_ENDIAN_BIG,
       // .color_space    = ESP_LCD_COLOR_SPACE_BGR,
        //.rgb_endian     = LCD_RGB_DATA_ENDIAN_BIG,
    };

    ret = esp_lcd_new_panel_ili9341(lcd_io_handle, &panel_config, &lcd_panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_ili9341 failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel_handle, false));
    //esp_lcd_panel_swap_xy(lcd_panel_handle, false);
    //esp_lcd_panel_mirror(lcd_panel_handle, true, false);

    // Backlight on - LCD_PIN_BL connects via 10 ohm resistor to ILI9341 LED pin
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << LCD_BL_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    gpio_set_level(LCD_BL_PIN, 1);


    ESP_LOGI(TAG, "Display initialized (MOSI=%d MISO=%d CLK=%d CS=%d DC=%d RST=%d BL=%d)",
             SPI2_MOSI_PIN, SPI2_MISO_PIN, SPI2_CLK_PIN,
             LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_BL_PIN);



    

    return ESP_OK;
}