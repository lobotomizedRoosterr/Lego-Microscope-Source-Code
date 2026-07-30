#include "components/include/mode_registry.h"
#include "esp_err.h"
#include "components/include/led_matrix.h"
#include "esp_heap_caps.h"
#include "components/include/frame_pool.h"
#include "computation/include/computation.h"
#include "esp_camera.h"

//these are holders for microscopy mode parameters (patterns, exposure, gain, etc)

microscopy_mode df_mode;
microscopy_mode bf_mode;
microscopy_mode qdf_mode;
microscopy_mode dpc_mode_lr;
microscopy_mode dpc_mode_rl;
microscopy_mode dpc_mode_tb;
microscopy_mode dpc_mode_bt;

int current_mode = BF_MODE; // sets it to default mode

// TODO: reuse TB for TB and BT, same for LR, RL (DPC)

/*
 * Note that for each pattern, it is simply 8 8-bit integers. Each bit represents whether an LED should be lit up.
 * This results in 64 bits, one for each LED
*/

/*
 * defines patterns for darkfield
*/
static const uint8_t df_pattern[] = {
    0b00111100,
    0b01111110,
    0b11100111,
    0b11000011,
    0b11000011,
    0b11100111,
    0b01111110,
    0b00111100
};
/*
 * defines patterns for brightfield
*/
static const uint8_t bf_pattern[] = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100
};
/*
 * defines patterns for qdf
*/
static const uint8_t qdf_patterns[] = {
    0b00110000,
    0b01110000,
    0b11100000,
    0b11000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00001100,
    0b00001110,
    0b00000111,
    0b00000011,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000011,
    0b00000111,
    0b00001110,
    0b00001100,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11000000,
    0b11100000,
    0b01110000,
    0b00110000,
};
/*
 * defines patterns for dpc bt also used by tb
*/
static const uint8_t dpc_patterns_bt[] = {
      
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000

};
/*
 * defines patterns for dpc lr. also used by rl
*/


static const uint8_t dpc_patterns_lr[] = {

    0b00110000,
    0b01110000,
    0b11110000,
    0b11110000,
    0b11110000,
    0b11110000,
    0b01110000,
    0b00110000,
    
    0b00001100,
    0b00001110,
    0b00001111,
    0b00001111,
    0b00001111,
    0b00001111,
    0b00001110,
    0b00001100,
};
/*
static const uint8_t dpc_patterns_tb[] = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,

    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100
};

*/
microscopy_mode get_mode_from_id(int id) {
    // returns actual struct based on id
    switch(id) {
        case DF_MODE:
        return df_mode;
        case BF_MODE:
        return bf_mode;
        case QDF_MODE:
        return qdf_mode;
        case DPC_LR_MODE:
        return dpc_mode_lr;
        case DPC_RL_MODE:
        return dpc_mode_rl;
        case DPC_TB_MODE:
        return dpc_mode_tb;
        case DPC_BT_MODE:
        return dpc_mode_bt;
    }
    return df_mode;
}

void get_pattern_from_index(int index, uint8_t pattern[8], microscopy_mode* mode) {
    // is mode and pattern valid?
    if (mode == NULL || pattern == NULL) {
        ESP_LOGE("MODE_REGISTRY", "Invalid mode or pattern buffer");
        return;
    }
    // is index within bounds?
    if (index < 0 || index >= mode->num_patterns) {
        ESP_LOGE("MODE_REGISTRY", "Invalid pattern index: %d", index);
        return;
    }
    //copy that data to the given parameter (pattern)
    memcpy(pattern, mode->matrix_patterns + index * 8, 8);
}

void store_matrix_patterns(microscopy_mode* mode, const uint8_t* patterns, size_t num_patterns) {
    size_t total_size = num_patterns * 8 * sizeof(uint8_t); // required size of buffer for holding patterns in bytes. number of patterns * 8 (1 byte for each row) * bytes per pixel (1)
    
    //allocate space for the patterns
    mode->matrix_patterns = heap_caps_malloc(total_size, MALLOC_CAP_SPIRAM); // Put in PSRAM, no DMA channel
    //matrix patterns valid?
    if (mode->matrix_patterns == NULL) {
        ESP_LOGE("MODE_REGISTRY", "Failed to allocate memory for matrix patterns");
        return;
    }
    //copy pattern to matrix pattern stored
    memcpy(mode->matrix_patterns, patterns, total_size);
}

void set_mode(int mode) {
    microscopy_mode m = get_mode_from_id(mode);
    set_pool_size(m.num_patterns);
    ESP_LOGI("Mode Registry", "Changing mode to %s", m.label);
    current_mode = mode;

    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_agc_gain(s, m.gain_val);
        s->set_aec_value(s, m.exposure_val);
        s->set_sharpness(s, m.sharpness_val);
    }
}

esp_err_t init_mode_registry() {
    esp_err_t ret = ESP_OK;

    memset(&df_mode, 0, sizeof(df_mode));
    memset(&bf_mode, 0, sizeof(bf_mode));
    memset(&qdf_mode, 0, sizeof(qdf_mode));
    memset(&dpc_mode_lr, 0, sizeof(dpc_mode_lr));
    memset(&dpc_mode_rl, 0, sizeof(dpc_mode_rl));
    memset(&dpc_mode_tb, 0, sizeof(dpc_mode_tb));
    memset(&dpc_mode_bt, 0, sizeof(dpc_mode_bt));

    df_mode.num_patterns=1;
    bf_mode.num_patterns=1;
    df_mode.matrix_patterns = (uint8_t*)df_pattern;
    bf_mode.matrix_patterns = (uint8_t*)bf_pattern;
    // push matrix patterns to matrix_patterns for qdf_mode
    store_matrix_patterns(&qdf_mode, qdf_patterns, 4);
    qdf_mode.num_patterns=4;

    df_mode.label="Darkfield";
    df_mode.short_label="DF";
    bf_mode.label="Brightfield";
    bf_mode.short_label="BF";
    qdf_mode.label="Quadrant Darkfield";
    qdf_mode.short_label="QDF";

    dpc_mode_lr.label="Differential Phase Contrast, LR";
    dpc_mode_lr.short_label="DPC LR";
    dpc_mode_lr.num_patterns=2;
    store_matrix_patterns(&dpc_mode_lr, dpc_patterns_lr, 2);
    
    dpc_mode_rl.label="Differential Phase Contrast, RL";
    dpc_mode_rl.short_label="DPC RL";
    dpc_mode_rl.num_patterns=2;
    store_matrix_patterns(&dpc_mode_rl, dpc_patterns_lr, 2);

    dpc_mode_tb.label="Differential Phase Contrast, TB";
    dpc_mode_tb.short_label="DPC TB";
    dpc_mode_tb.num_patterns=2;
    store_matrix_patterns(&dpc_mode_tb, dpc_patterns_bt, 2);

    dpc_mode_bt.label="Differential Phase Contrast, BT";
    dpc_mode_bt.short_label="DPC BT";
    dpc_mode_bt.num_patterns=2;
    store_matrix_patterns(&dpc_mode_bt, dpc_patterns_bt, 2);


    //set exposure, exposure times, gain, etc.
    df_mode.exposure_time=20;
    df_mode.exposure_val=75;
    df_mode.gain_val=1;
    df_mode.sharpness_val=2;
    df_mode.led_brightness_val=15;

    
    bf_mode.exposure_time=2;
    bf_mode.exposure_val=400;
    bf_mode.gain_val=0;
    bf_mode.sharpness_val=2;
    bf_mode.led_brightness_val=10;

    
    qdf_mode.exposure_time=20;
    qdf_mode.exposure_val=250;
    qdf_mode.gain_val=0;
    qdf_mode.sharpness_val=2;
    qdf_mode.led_brightness_val=15;

    
    dpc_mode_lr.exposure_time=10;
    dpc_mode_lr.exposure_val=75;
    dpc_mode_lr.gain_val=0;
    dpc_mode_lr.sharpness_val=-2;
    dpc_mode_lr.led_brightness_val=15;

    
    dpc_mode_rl.exposure_time=10;
    dpc_mode_rl.exposure_val=75;
    dpc_mode_rl.gain_val=0;
    dpc_mode_rl.sharpness_val=-2;
    dpc_mode_rl.led_brightness_val=15;

    
    dpc_mode_tb.exposure_time=10;
    dpc_mode_tb.exposure_val=75;
    dpc_mode_tb.gain_val=0;
    dpc_mode_tb.sharpness_val=-2;
    dpc_mode_tb.led_brightness_val=15;

    dpc_mode_bt.exposure_time=10;
    dpc_mode_bt.exposure_val=75;
    dpc_mode_bt.gain_val=0;
    dpc_mode_bt.sharpness_val=-2;
    dpc_mode_bt.led_brightness_val=15;


    led_array_send_matrix(df_pattern);

    set_mode(DF_MODE);

    return ret;
}
