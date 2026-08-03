#include "computation/include/computation.h"

#include "components/include/mode_registry.h"
#include "components/include/frame_pool.h"
#include "components/include/render_engine.h"
#include "components/include/acquisition_sequencer.h"
#include "components/include/camera.h"

#include "main/app_state.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "components/include/ui.h"

#include "esp_lcd_panel_ops.h"
#include "freertos/task.h"

#include <math.h>
#include "esp_dsp.h"
#include "esp_random.h"

static const char* TAG = "COMPUTATION";

ESP_EVENT_DECLARE_BASE(NEW_FRAME_POOL_PROCESSED);

size_t frame_size = (size_t) 0;

float c_scale = 0.9f; // Default scaling factor for QDF computation

uint8_t* computation_frame;

SemaphoreHandle_t computation_frame_mutex;

int frames;

/*
Think about having every pixel value being a float, 
then placing each pixel as an int value from 0 to 255, 
but normalizing them based on range. 
(perhaps highest float value becomes 255, lowest becomes 0, etc.) 
Or something like that
*/

void begin_new_computation(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    if (xSemaphoreTake(computation_frame_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }
    finished_frame_pool *fp = (finished_frame_pool*) event_data;
    if (fp == NULL || fp->data == NULL) {
        ESP_LOGE(TAG, "begin_new_computation received invalid frame pool");
        return;
    }
    switch (current_mode) {
        case BF_MODE:
            run_bf(&fp->data, &computation_frame);
            break;
        case DF_MODE:
            run_df(&fp->data, &computation_frame);
            break;
        case QDF_MODE:
            run_qdf(&fp->data, &computation_frame, c_scale);
            break;
        case DPC_LR_MODE:
            run_dpc_lr(&fp->data, &computation_frame);
            break;
        case DPC_RL_MODE:
            run_dpc_rl(&fp->data, &computation_frame);
            break;
        case DPC_TB_MODE:
            run_dpc_tb(&fp->data, &computation_frame);
            break;
        case DPC_BT_MODE:
            run_dpc_bt(&fp->data, &computation_frame);
            break;
        default:
            ESP_LOGE(TAG, "Unknown mode: %d", current_mode);
            return;
    }
    if (computation_frame == NULL) {
        ESP_LOGE(TAG, "computation_frame is NULL after mode processing, skipping render");
        return;
    }
    //draw_greyscale_image(computation_frame, 0, 0, 320, 240);
    xSemaphoreGive(computation_frame_mutex);
    esp_event_post_to(event_loop_handle, NEW_FRAME_POOL_PROCESSED, NEW_FRAME_POOL_PROCESSED_ID, NULL, 0, 10);
}

void run_bf(uint8_t** data, uint8_t** buf) {
    const uint8_t* img = *data;
    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float val_f = (float) img[idx] / (255.0f/63.0f);
            int val = (int) val_f;
            if(val < 0) val = 0;
            if(val > 63) val = 63;

            (*buf)[idx] = (uint8_t)val;
        }
    }
}

void run_df(uint8_t** data, uint8_t** buf) {
    const uint8_t* img = *data;


    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float val_f = (float) img[idx] / (255.0f/63.0f);
            int val = (int) val_f;
            if(val < 0) val = 0;
            if(val > 63) val = 63;

            (*buf)[idx] = (uint8_t)val;
        }
    }
}

void run_qdf(uint8_t** data, uint8_t** buf, float c_scale) {

    uint8_t* qdf_image = heap_caps_malloc(frame_size, MALLOC_CAP_SPIRAM);
    if (qdf_image == NULL) {
        ESP_LOGE(TAG, "Failed to allocate QDF image buffer");
        return;
    }
    //TODO: Look at led orientation stuff.

    const uint8_t* tl_img = *data;
    const uint8_t* tr_img = *data + frame_size;
    const uint8_t* br_img = *data + frame_size * 2;
    const uint8_t* bl_img = *data + frame_size * 3;

    for (int y = 0; y < capture_height; y++) {
        for (int x = 0; x < capture_width; x++) {

            size_t idx = (size_t)y * capture_width + x;

            float tl = (float) tl_img[idx]/(255.0f/63.0f);
            float tr = (float) tr_img[idx]/(255.0f/63.0f);
            float br = (float) br_img[idx]/(255.0f/63.0f);
            float bl = (float) bl_img[idx]/(255.0f/63.0f);

            // --- Compute edge image (Eq. 1) ---
            float E = fabsf(tl - br) + fabsf(bl - tr);

            // --- Compute darkfield image ---
            float DF = tl + tr + br + bl;

            // --- Compute QDF (Eq. 2) ---
            float qdf_val = (c_scale * DF - E);

            // Clamp to 0-255
            if (qdf_val < 0) qdf_val = 0;
            if (qdf_val > 255) qdf_val = 255;

            qdf_image[idx] = (uint8_t)qdf_val;

        }
    }
    memcpy(*buf, qdf_image, frame_size);
    heap_caps_free(qdf_image);
}

void run_dpc_lr(uint8_t** data, uint8_t** buf) {
    const uint8_t* l_img = *data;
    const uint8_t* r_img = *data + frame_size;

    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float l = (float) l_img[idx];
            float r = (float) r_img[idx];

            
            float num = l - r;
            // add small number to denominator to prevent divide by zero errors
            float den = (l + r) + 0.0001f;
            // DPC values range between -1 and 1. To change to 0-255, I added 1, divided by 2 and multiplied by 255.
            float val_f = ((num/den) + 1.0f)/2*63.0f;
            int val = (int) val_f;


            if(val > 63) val  = 63;
            if(val < 0) val = 0;

            (*buf)[idx] = (uint8_t) val;
        }
    }
}

void run_dpc_rl(uint8_t** data, uint8_t** buf) {
    const uint8_t* l_img = *data;
    const uint8_t* r_img = *data + frame_size;

    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float l = (float) l_img[idx]/(255.0f/63.0f);
            float r = (float) r_img[idx]/(255.0f/63.0f);

            float num = r - l;
            // add small number to denominator to prevent divide by zero errors
            float den = (r + l) + 0.0001f;
            // DPC values range between -1 and 1. To change to 0-255, I added 1, divided by 2 and multiplied by 255.
            float val_f = ((num/den) + 1.0f) / 2.0f * 63.0f;
            int val = (int) val_f;
            if(val < 0) val = 0;
            if(val > 255) val = 255;

            (*buf)[idx] = (uint8_t)val;
        }
    }
}
//when well is full, record max value. Something weird in math, intensity too high.

void run_dpc_tb(uint8_t** data, uint8_t** buf) {
    const uint8_t* t_img = *data;
    const uint8_t* b_img = *data + frame_size;

    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float b = (float) b_img[idx];
            float t = (float) t_img[idx];

            float num = t - b;
            
            // add small number to denominator to prevent divide by zero errors
            float den = (t + b) + 0.0001f;
            // DPC values range between -1 and 1. To change to 0-255, I added 1, divided by 2 and multiplied by 255.
            float val_f = ((num/den) + 1.0f) / 2.0f * 63.0f;
            int val = (int) val_f;

            if(val < 0) val = 0;
            if(val > 255) val = 255;

            (*buf)[idx] = (uint8_t)val;
        }
    }
}

void run_dpc_bt(uint8_t** data, uint8_t** buf) {
    const uint8_t* t_img = *data;
    const uint8_t* b_img = *data + frame_size;

    for(int x = 0; x < capture_width; x++) {
        for(int y = 0; y < capture_height; y++) {
            size_t idx = (size_t)y * capture_width + x;
            float b = (float) b_img[idx];
            float t = (float) t_img[idx];

            float num = b - t;
            // add small number to denominator to prevent divide by zero errors
            float den = (b + t) + 0.0001f;
            // DPC values range between -1 and 1. To change to 0-255, I added 1, divided by 2 and multiplied by 255.
            float val_f = ((num/den) + 1.0f) / 2.0f * 63.0f;
            int val = (int) val_f;

            if(val < 0) val = 0;
            if(val > 255) val = 255;

            (*buf)[idx] = (uint8_t)val;
        }
    }
}

esp_err_t init_computation() {
    esp_err_t ret = ESP_OK;

    // size is only 320X240 bytes, because the actual frame passed to frame buffer is greyscale.

    frame_size = (size_t) capture_width*capture_height;

    //allocate a buffer for the computation frame.
    computation_frame = heap_caps_malloc(frame_size, MALLOC_CAP_SPIRAM);
    if (computation_frame == NULL) {
        ESP_LOGE(TAG, "Failed to allocate initial computation_frame");
        return ESP_ERR_NO_MEM;
    }

    computation_frame_mutex=xSemaphoreCreateMutex();

    return ret;
}
