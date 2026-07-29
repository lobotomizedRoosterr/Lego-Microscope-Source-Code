#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_err.h"
#include <stdbool.h>

#define EXPOSURE_TIME_MS 20

extern int current_pattern_index; 
extern volatile bool calibration_in_progress;


void camera_acquisition_task(void *arg);

esp_err_t init_acquisition_sequencer();
