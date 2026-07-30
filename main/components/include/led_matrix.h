#include "esp_err.h"

/* initiates LED matrix (connecting to spi3 bus, etc.)*/
esp_err_t init_led_matrix();

/*write data to a specific register
 * @param reg register to write to
 * @param data data to write to the register
 * @returns returns an error if it failed
*/
esp_err_t write_reg(uint8_t reg, uint8_t data);

/*
 * sets the intensity (brightness) of led matrix
 * @param intensity value from 0-15 for brightness
 * @returns returns an error if fails
*/
esp_err_t led_array_set_intensity(uint8_t intensity);
 
/*
 * clears the led matrix
 * @returns returns error if it fails
*/
esp_err_t led_array_clear(void);
 
/*
 * Sends matrix data to the led array
 * @param matrix matrix pattern to send
 * @returns returns an error if it fails
*/
esp_err_t led_array_send_matrix(const uint8_t matrix[8]);
 
