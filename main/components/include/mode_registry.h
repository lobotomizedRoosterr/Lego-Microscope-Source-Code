#include "esp_err.h"
#include <string.h>
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#define BF_MODE 0 /*Brightfield mode*/
#define DF_MODE 1 /*Darkfield mode*/
#define QDF_MODE 2 /*Quadrant Darkfield mode*/
#define DPC_LR_MODE 3 /*Differential Phase Contrast (L-R)/(L+R)*/
#define DPC_RL_MODE 4 /*Differential Phase Contrast (R-L)/(R+L)*/
#define DPC_TB_MODE 5 /*Differential Phase Contrast (T-B)/(T+B)*/
#define DPC_BT_MODE 6 /*Differential Phase Contrast (B-T)/(B+T)*/

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
