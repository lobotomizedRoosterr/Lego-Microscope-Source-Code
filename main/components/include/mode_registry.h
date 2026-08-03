#include "esp_err.h"
#include <string.h>
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#define NUM_MODES 8 /* mode IDs run 0..7 inclusive — was 7, off-by-one */

#define BF_MODE 0 /*Brightfield mode*/
#define DF_MODE 1 /*Darkfield mode*/
#define QDF_MODE 2 /*Quadrant Darkfield mode*/
#define DPC_GEN_MODE 3
#define DPC_LR_MODE 4 /*Differential Phase Contrast (L-R)/(L+R)*/
#define DPC_RL_MODE 5 /*Differential Phase Contrast (R-L)/(R+L)*/
#define DPC_TB_MODE 6 /*Differential Phase Contrast (T-B)/(T+B)*/
#define DPC_BT_MODE 7 /*Differential Phase Contrast (B-T)/(B+T)*/


/*
 * this defines the current microscopy mode id that is set for the system
*/
extern int current_mode;

/*
 * this stores configuration data for every microscopy mode
*/
typedef struct {
    uint8_t* matrix_patterns; /* matrix pattern data*/
    char* label; /* long label that describes the mode (ex. Darkfield mode)*/
    char* short_label; /* short label that represents the mode (ex. DF)*/
    int num_patterns; /* number of patterns to loop through on the led array*/
    int exposure_val; /* Camera config for exposure 0-1200*/
    int gain_val; /*camera config for gain 0-30*/
    int exposure_time; /*time to wait between setting led array and capturing a picture*/
    int sharpness_val; /*camera config for sharpness -2 - 2*/
    int led_brightness_val; /*brightness of led matrix 0-15*/

} microscopy_mode;

/*
 * Persisted subset of microscopy_mode — just the camera/illumination
 * tuning parameters. This is what actually gets read from / written to
 * flash; patterns and labels stay compiled into the firmware.
*/
typedef struct {
    int exposure_val;
    int gain_val;
    int exposure_time;
    int sharpness_val;
    int led_brightness_val;
} microscopy_config;

/* this runs everything needed when setting mode.*/
void set_mode(int mode);

/*
 * Retrieves a pattern based on mode and index. It seperates a specific LED pattern from the 
 * general "matrix_patterns" variable from microscopy_mode, which stores every pattern
 * @param index index of pattern to find (ex. 0 - first pattern, 1 - second pattern)
 * @param pattern variable where the foudn pattern will be stored.
 * @param mode microscopy mode whose pattern we are finding
*/
void get_pattern_from_index(int index, uint8_t pattern[8], microscopy_mode* mode);
/*
 * initializes mode registry
*/
esp_err_t init_mode_registry();

/*
 * gets mode based on its id.
*/
microscopy_mode get_mode_from_id(int id);

/*
 * Mounts the SPIFFS config partition (if not already mounted elsewhere)
 * and loads persisted camera config for every mode into the in-memory
 * mode structs. If no config file exists yet (first boot / fresh
 * flash), writes out whatever is currently in the mode structs as the
 * defaults, so subsequent boots read back from flash.
 *
 * Call this from init_mode_registry(), after the hardcoded
 * exposure/gain/etc. defaults have been assigned to each mode struct,
 * and before set_mode() is called to apply settings to the sensor.
*/
esp_err_t mode_config_init(void);

/*
 * Reads the currently persisted config for one mode directly from
 * flash, without touching the in-memory mode struct.
 * @param mode_id one of BF_MODE..DPC_BT_MODE
 * @param out_cfg destination for the loaded config
*/
esp_err_t mode_config_get(int mode_id, microscopy_config* out_cfg);

/*
 * Updates the in-memory mode struct's camera config fields AND
 * persists the change to flash immediately, so it survives reboot.
 * If mode_id is the currently active mode, also re-applies the new
 * gain/exposure/sharpness to the camera sensor right away.
 * @param mode_id one of BF_MODE..DPC_BT_MODE
 * @param cfg the new config values to apply and persist
*/
esp_err_t mode_config_set(int mode_id, const microscopy_config* cfg);

/*
 * Overwrites the on-flash config for every mode with whatever is
 * currently in the in-memory mode structs.
*/
esp_err_t mode_config_save_all(void);

/*
 * Erases the on-flash config file. The next mode_config_init() call
 * will regenerate it from whatever defaults are in the mode structs
 * at that time (normally the hardcoded init_mode_registry() values).
*/
esp_err_t mode_config_reset_defaults(void);