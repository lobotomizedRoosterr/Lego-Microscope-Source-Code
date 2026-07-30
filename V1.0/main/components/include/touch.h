#include "esp_err.h"
#include <stdint.h>
#include "stdbool.h"

/*
 * Just used to stored touch data
*/
typedef struct {
    uint16_t touch_x; /*x value of last touch position*/
    uint16_t touch_y; /*y value of last touch position*/
    uint16_t touch_strength; /*(for resistive touch) detected force from last touch*/
    bool pressed; /*if the user is currently pressing on the screen (if false, other data refers to the last time the user touched the screen)*/
} touch_data;

/*General initialization for touch, connect XPT2046 to SPI Bus*/
esp_err_t init_touch(void);

/*Read from touch data, return in a struct*/
touch_data get_touch();
