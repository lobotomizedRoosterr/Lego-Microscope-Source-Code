#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_err.h"
#include <stdbool.h>

#define EXPOSURE_TIME_MS 20 /*default time to wait between setting LED array and capturing an image*/

/*Index of current pattern*/
extern int current_pattern_index; 

/*run the loop for camera acquisition*/
void camera_acquisition_task(void *arg);

/*initialize acquisition sequencer*/
esp_err_t init_acquisition_sequencer();
