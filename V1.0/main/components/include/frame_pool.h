#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_camera.h"

/* This is the number of frames each frame pool has, given what mode is selected. (ex. 4 for QDF, 2 for DPC, etc.)*/
extern int frame_pool_size;

/*this stores data and information about the frame pool when finished*/
typedef struct {
    uint8_t *data; /*this contains the greyscale images.*/
    int      frame_count; /*number of frames passed*/
    int      frame_width; /*width of each frame*/
    int      frame_height; /*height of each frame*/
    uint8_t *pattern_indices;   /*indices dictating which frame corresponds to image taken during a specific pattern*/
} finished_frame_pool;

/*Here is stored the finished frame pool*/
extern finished_frame_pool new_fp;
/*here dictates if image processing is done, and frame pools can be swapped*/
extern volatile bool can_frame_pool_swap;

/*initializes frame pool. creates buffers, allocates memory, etc.*/
esp_err_t init_frame_pool(void);
/*This configures the frame pool to allow for a specific number of frames*/
void set_pool_size(int frame_count);

/*
 * This pushes a frame to the pool
 * @param fb camera framebuffer to push
 * @param format format to push the frame in
 * @param pattern_index index of the pattern that is being pushed (what pattern was active when capture took place)
 * @returns returns a value that describes success of push
*/
int push_frame_to_pool(camera_fb_t* fb, int format, int pattern_index);
/*
 * This is specifically for an event it is called when the event that states that the frame pool is finished processing has been triggerred.
 * 
*/
void new_frame_pool_processed(void *handler_arg,
                              esp_event_base_t base,
                              int32_t id,
                              void *event_data);
