#include "components/include/mode_registry.h"
#include "esp_err.h"
#include "components/include/led_matrix.h"
#include "esp_heap_caps.h"
#include "components/include/frame_pool.h"
#include "computation/include/computation.h"
#include "esp_camera.h"
#include "esp_spiffs.h"
#include <stdio.h>

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

/* ---- SPIFFS-backed camera config storage ---- */
/* Reuses the "storage" partition/mount point that image_store already
 * registers; adjust these if your project uses a different label. */
static const char* CFG_PARTITION_LABEL = "storage";
static const char* CFG_MOUNT_POINT = "/storage";
static const char* CFG_FILE_PATH = "/storage/mode_cfg.bin";
static const char* CFG_TAG = "MODE_CONFIG";

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
 * Returns a POINTER to the actual static mode struct for a given id
 * (unlike get_mode_from_id(), which returns a copy). Internal use only —
 * needed so mode_config_* functions can mutate the live struct in place.
*/
static microscopy_mode* get_mode_ptr_from_id(int id) {
    switch (id) {
        case DF_MODE:
        return &df_mode;
        case BF_MODE:
        return &bf_mode;
        case QDF_MODE:
        return &qdf_mode;
        case DPC_LR_MODE:
        return &dpc_mode_lr;
        case DPC_RL_MODE:
        return &dpc_mode_rl;
        case DPC_TB_MODE:
        return &dpc_mode_tb;
        case DPC_BT_MODE:
        return &dpc_mode_bt;
    }
    // No backing struct for this id (e.g. DPC_GEN_MODE) — callers that
    // iterate 0..NUM_MODES-1 must skip ids like this via has_mode_struct().
    ESP_LOGE("MODE_REGISTRY", "get_mode_ptr_from_id: no struct for id %d", id);
    return &df_mode;
}

static bool has_mode_struct(int id) {
    return id != DPC_GEN_MODE;
}

microscopy_mode get_mode_from_id(int id) {
    return *get_mode_ptr_from_id(id);
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

/* ---- SPIFFS-backed camera config storage ---- */

static esp_err_t ensure_spiffs_mounted(void) {
    if (esp_spiffs_mounted(CFG_PARTITION_LABEL)) {
        return ESP_OK;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = CFG_MOUNT_POINT,
        .partition_label = CFG_PARTITION_LABEL,
        .max_files = 4,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(CFG_TAG, "Failed to mount SPIFFS partition '%s': %s",
                 CFG_PARTITION_LABEL, esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(CFG_PARTITION_LABEL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(CFG_TAG, "SPIFFS mounted: %d/%d bytes used", used, total);
    }
    return ESP_OK;
}

static void config_from_mode(const microscopy_mode* mode, microscopy_config* cfg) {
    cfg->exposure_val = mode->exposure_val;
    cfg->gain_val = mode->gain_val;
    cfg->exposure_time = mode->exposure_time;
    cfg->sharpness_val = mode->sharpness_val;
    cfg->led_brightness_val = mode->led_brightness_val;
}

static void apply_config_to_mode(microscopy_mode* mode, const microscopy_config* cfg) {
    mode->exposure_val = cfg->exposure_val;
    mode->gain_val = cfg->gain_val;
    mode->exposure_time = cfg->exposure_time;
    mode->sharpness_val = cfg->sharpness_val;
    mode->led_brightness_val = cfg->led_brightness_val;
}
esp_err_t mode_config_save_all(void) {
    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    FILE* f = fopen(CFG_FILE_PATH, "wb");
    if (f == NULL) {
        ESP_LOGE(CFG_TAG, "Failed to open '%s' for writing", CFG_FILE_PATH);
        return ESP_FAIL;
    }

    for (int id = 0; id < NUM_MODES; id++) {
        microscopy_config cfg = {0};
        if (has_mode_struct(id)) {
            config_from_mode(get_mode_ptr_from_id(id), &cfg);
        }
        // unmapped ids (DPC_GEN_MODE) still get a placeholder record
        // written so every slot's file offset stays fixed
        if (fwrite(&cfg, sizeof(microscopy_config), 1, f) != 1) {
            ESP_LOGE(CFG_TAG, "Failed writing config for mode %d", id);
            fclose(f);
            return ESP_FAIL;
        }
    }

    fclose(f);
    ESP_LOGI(CFG_TAG, "Saved config for all %d modes to flash", NUM_MODES);
    return ESP_OK;
}

esp_err_t mode_config_get(int mode_id, microscopy_config* out_cfg) {
    if (out_cfg == NULL || mode_id < 0 || mode_id >= NUM_MODES) {
        ESP_LOGE(CFG_TAG, "Invalid arguments to mode_config_get (mode_id=%d)", mode_id);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    FILE* f = fopen(CFG_FILE_PATH, "rb");
    if (f == NULL) {
        ESP_LOGE(CFG_TAG, "Config file '%s' not found — call mode_config_init() first", CFG_FILE_PATH);
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(f, mode_id * sizeof(microscopy_config), SEEK_SET) != 0) {
        fclose(f);
        return ESP_FAIL;
    }

    size_t read = fread(out_cfg, sizeof(microscopy_config), 1, f);
    fclose(f);

    if (read != 1) {
        ESP_LOGE(CFG_TAG, "Failed reading config for mode %d", mode_id);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t mode_config_set(int mode_id, const microscopy_config* cfg) {
    if (cfg == NULL || mode_id < 0 || mode_id >= NUM_MODES) {
        ESP_LOGE(CFG_TAG, "Invalid arguments to mode_config_set (mode_id=%d)", mode_id);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    // update in-memory struct first
    microscopy_mode* mode = get_mode_ptr_from_id(mode_id);
    apply_config_to_mode(mode, cfg);

    // then persist just this mode's record
    FILE* f = fopen(CFG_FILE_PATH, "r+b");
    if (f == NULL) {
        // file may not exist yet — fall back to writing out everything
        return mode_config_save_all();
    }

    if (fseek(f, mode_id * sizeof(microscopy_config), SEEK_SET) != 0) {
        fclose(f);
        return ESP_FAIL;
    }

    size_t written = fwrite(cfg, sizeof(microscopy_config), 1, f);
    fclose(f);

    if (written != 1) {
        ESP_LOGE(CFG_TAG, "Failed persisting config for mode %d", mode_id);
        return ESP_FAIL;
    }

    ESP_LOGI(CFG_TAG, "Updated and persisted config for mode %d", mode_id);

    // if this is the live mode, re-apply the new values to the sensor now
    if (mode_id == current_mode) {
        sensor_t *s = esp_camera_sensor_get();
        if (s != NULL) {
            s->set_agc_gain(s, mode->gain_val);
            s->set_aec_value(s, mode->exposure_val);
            s->set_sharpness(s, mode->sharpness_val);
        }
    }

    return ESP_OK;
}

esp_err_t mode_config_reset_defaults(void) {
    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    if (remove(CFG_FILE_PATH) != 0) {
        ESP_LOGW(CFG_TAG, "No existing config file to remove (or removal failed)");
    }
    return ESP_OK;
}

esp_err_t mode_config_init(void) {
    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    FILE* f = fopen(CFG_FILE_PATH, "rb");
    if (f == NULL) {
        // first boot / fresh flash — seed the file from current in-memory defaults
        ESP_LOGI(CFG_TAG, "No config file found, seeding defaults");
        return mode_config_save_all();
    }

    for (int id = 0; id < NUM_MODES; id++) {
        microscopy_config cfg;
        if (fread(&cfg, sizeof(microscopy_config), 1, f) != 1) {
            ESP_LOGW(CFG_TAG, "Config file too short at mode %d, keeping hardcoded default and re-saving", id);
            fclose(f);
            return mode_config_save_all();
        }
        if (has_mode_struct(id)) {
            apply_config_to_mode(get_mode_ptr_from_id(id), &cfg);
        }
        // ids without a struct (DPC_GEN_MODE) are read to advance the
        // file cursor correctly, then discarded
    }

    fclose(f);
    ESP_LOGI(CFG_TAG, "Loaded config for all %d modes from flash", NUM_MODES);
    return ESP_OK;
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


    //set exposure, exposure times, gain, etc. — these are the HARDCODED
    //DEFAULTS. mode_config_init() below will overwrite them with
    //flash-persisted values if a config file already exists.
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

    // load persisted config from flash (or seed flash with the defaults
    // above, on first boot) — must run after hardcoded defaults, before
    // set_mode() applies values to the sensor
    ret = mode_config_init();
    if (ret != ESP_OK) {
        ESP_LOGW("MODE_REGISTRY", "mode_config_init failed (%s), continuing with hardcoded defaults", esp_err_to_name(ret));
        ret = ESP_OK; // non-fatal — hardcoded values are still valid
    }

    led_array_send_matrix(df_pattern);

    set_mode(DF_MODE);

    return ret;
}