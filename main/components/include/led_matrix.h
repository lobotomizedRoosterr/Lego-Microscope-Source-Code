#include "esp_err.h"

esp_err_t init_led_matrix();

esp_err_t write_reg(uint8_t reg, uint8_t data);
esp_err_t led_array_set_intensity(uint8_t intensity);
 
esp_err_t led_array_clear(void);
 
esp_err_t led_array_send_matrix(const uint8_t matrix[8]);
 