#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_camera.h"

extern int frame_pool_size;

typedef struct {
    uint8_t *data;
    int      frame_count;
    int      frame_width;
    int      frame_height;
    uint8_t *pattern_indices;   // NEW: pattern_indices[i] = which pattern slot i corresponds to
} finished_frame_pool;

extern finished_frame_pool new_fp;
extern volatile bool can_frame_pool_swap;

esp_err_t init_frame_pool(void);
void set_pool_size(int frame_count);


// Update push_frame_to_pool signature to accept the pattern index being captured:
int push_frame_to_pool(camera_fb_t* fb, int format, int pattern_index);
void new_frame_pool_processed(void *handler_arg,
                              esp_event_base_t base,
                              int32_t id,
                              void *event_data);