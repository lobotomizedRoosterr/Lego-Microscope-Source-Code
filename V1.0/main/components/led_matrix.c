#include "components/include/led_matrix.h"
#include "components/include/spi_bus_manager.h"
#include "main/pin_config.h"
#include "main/app_state.h"
#include "esp_err.h"

#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

/*
TODO : IMPLEMENT ORIENTATION OF ARRAY WHEN DISPLAYING STUFF
DONE : orientation is now handled via LED_MATRIX_ORIENTATION below.
*/

// Dedicated SPI3 bus — independent from ILI9341 on SPI2
#define MAX7219_SPI_HOST    SPI3_HOST

/*
   Set to how the physical matrix is mounted relative to "logical" 
   row 0 = top, col 0 = left (bit 0 = leftmost column of each row).
   Rotation is applied clockwise, in software, before every send.
*/
#define LED_MATRIX_ROT_0    0
#define LED_MATRIX_ROT_90   1   // 90° clockwise
#define LED_MATRIX_ROT_180  2
#define LED_MATRIX_ROT_270  3   // 270° clockwise (== 90° CCW)

#ifndef LED_MATRIX_ORIENTATION
#define LED_MATRIX_ORIENTATION LED_MATRIX_ROT_90
#endif

/* 
 * These are registers for different variables.
*/
#define REG_NOOP        0x00
#define REG_DIGIT0      0x01
#define REG_SCANLIMIT   0x0B
#define REG_DECODEMODE  0x09
#define REG_INTENSITY   0x0A
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAYTEST 0x0F

spi_device_handle_t max_spi = NULL;

static const char *TAG = "MAX7219";

/*
   Each function transforms an 8x8 bitmap (matrix[row] = 8 bits,
   bit 0 = leftmost column) into the equivalent bitmap rotated
   clockwise by the named amount.
*/

//rotates pattern none
static void rotate_0(const uint8_t in[8], uint8_t out[8]) {
    memcpy(out, in, 8);
}

//rotates pattern 90 degrees clockwise
static void rotate_90cw(const uint8_t in[8], uint8_t out[8]) {
    memset(out, 0, 8);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (in[row] & (1 << col)) {
                out[col] |= (1 << (7 - row));
            }
        }
    }
}

//rotates pattern 180 degrees
static void rotate_180(const uint8_t in[8], uint8_t out[8]) {
    for (int row = 0; row < 8; row++) {
        uint8_t src = in[row];
        uint8_t rev = 0;
        for (int col = 0; col < 8; col++) {
            if (src & (1 << col)) {
                rev |= (1 << (7 - col));
            }
        }
        out[7 - row] = rev;
    }
}

//rotates pattern 270 degrees clockwise (90 degrees counter-clockwise)
static void rotate_270cw(const uint8_t in[8], uint8_t out[8]) {
    memset(out, 0, 8);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (in[row] & (1 << col)) {
                out[7 - col] |= (1 << row);
            }
        }
    }
}

//macro for applying orientation
static void apply_orientation(const uint8_t in[8], uint8_t out[8]) {
#if LED_MATRIX_ORIENTATION == LED_MATRIX_ROT_90
    rotate_90cw(in, out);
#elif LED_MATRIX_ORIENTATION == LED_MATRIX_ROT_180
    rotate_180(in, out);
#elif LED_MATRIX_ORIENTATION == LED_MATRIX_ROT_270
    rotate_270cw(in, out);
#else
    rotate_0(in, out);
#endif
}

esp_err_t init_led_matrix() {
    esp_err_t ret = ESP_OK;
    // Configure CS pin manually — MAX7219 latches on rising CS edge,
    // which requires CS to go high AFTER the full 16-bit word.
    // The SPI driver's built-in CS toggles per transaction which is correct,
    // but we drive it manually for explicit control.
    gpio_config_t cs_cfg = {
        .pin_bit_mask = (1ULL << MAX_CS_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cs_cfg);
    gpio_set_level(MAX_CS_PIN, 1);  // deassert (idle high)

    spi_device_interface_config_t devcfg = {0};
    devcfg.clock_speed_hz = 1000000;    // 1 MHz
    devcfg.mode           = 0;          // CPOL=0, CPHA=0
    devcfg.spics_io_num   = -1;         // CS managed manually above
    devcfg.queue_size     = 1;

    //add device to spi bus
    ret = spi_bus_add_device(MAX7219_SPI_HOST, &devcfg, &max_spi);
    ESP_LOGI(TAG, "MAX7219: spi_bus_add_device -> %s", esp_err_to_name(ret));
    if (ret != ESP_OK) return ret;

    // Init sequence
    write_reg(REG_SCANLIMIT,   0x07);   // all 8 rows active
    write_reg(REG_DECODEMODE,  0x00);   // no BCD decode (raw bitmap mode)
    write_reg(REG_DISPLAYTEST, 0x00);   // normal operation
    write_reg(REG_SHUTDOWN,    0x01);   // wake up (0x00 = shutdown)
    write_reg(REG_INTENSITY,   0x09);   // high brightness (0–15)
    led_array_set_intensity(1);

    ESP_LOGI(TAG, "MAX7219 initialized successfully");
    return ESP_OK;
}

/* low level write*/
esp_err_t write_reg(uint8_t reg, uint8_t data) {
    // Manual CS: pull low, transmit, pull high
    gpio_set_level(MAX_CS_PIN, 0);

    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = (uint8_t[]){reg, data}
    };
    esp_err_t ret = spi_device_transmit(max_spi, &t);

    gpio_set_level(MAX_CS_PIN, 1);
    return ret;
}

//These functions configure the array by sending to specific registers.

esp_err_t led_array_set_intensity(uint8_t intensity) {
    if (intensity > 15) intensity = 15;
    return write_reg(REG_INTENSITY, intensity);
}

esp_err_t led_array_clear(void) {
    uint8_t empty[8] = {0};
    return led_array_send_matrix(empty);
}

esp_err_t led_array_send_matrix(const uint8_t matrix[8]) {
    //orient
    uint8_t oriented[8];
    apply_orientation(matrix, oriented);

    for (int row = 0; row < 8; row++) {
        // spi transaction for each row
        spi_transaction_t t = {
            .length    = 16,
            .tx_buffer = (uint8_t[]){row + 1, oriented[row]}
        };
        gpio_set_level(MAX_CS_PIN, 0);

        //send spi transaction
        ESP_ERROR_CHECK(spi_device_transmit(max_spi, &t));
        gpio_set_level(MAX_CS_PIN, 1);
    }

    return ESP_OK;
}
