#include "esp_err.h"
#include <stdint.h>
#include "stdbool.h"

typedef struct {
    uint16_t touch_x;
    uint16_t touch_y;
    uint16_t touch_strength;
    bool pressed;
} touch_data;

esp_err_t init_touch(void);

touch_data get_touch();