#include "esp_err.h"
#include <string.h>
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#define BF_MODE 0
#define DF_MODE 1
#define QDF_MODE 2
#define DPC_LR_MODE 3
#define DPC_RL_MODE 4
#define DPC_TB_MODE 5
#define DPC_BT_MODE 6

extern int current_mode;

typedef struct {
    uint8_t* matrix_patterns;
    char* label;
    char* short_label;
    int num_patterns;
    int exposure_val;
    int gain_val;
    int exposure_time;
    int sharpness_val;
    int led_brightness_val;

} microscopy_mode;

void set_mode(int mode);

void get_pattern_from_index(int index, uint8_t pattern[8], microscopy_mode* mode);
void bg_capture();
esp_err_t init_mode_registry();
microscopy_mode get_mode_from_id(int id);