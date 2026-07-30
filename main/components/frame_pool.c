#include "components/include/frame_pool.h"
#include "components/include/camera.h"
#include "computation/include/computation.h"
#include "components/include/render_engine.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "main/app_state.h"
#include "esp_event.h"

#include "esp_camera.h"
#include <assert.h>
#include <stdbool.h>

static uint8_t *frame_pool = NULL;
static uint8_t *full_frame_pool = NULL;

// NEW: parallel arrays tracking which pattern index each slot holds
static uint8_t *frame_pool_indices = NULL;
static uint8_t *full_frame_pool_indices = NULL;

static const char *TAG = "FRAME_POOL";

#define BYTES_PER_PIXEL 1

int frame_pool_size = 1;
static int frames_in_pool = 0;

volatile bool can_frame_pool_swap = true;

void new_frame_pool_processed(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    can_frame_pool_swap = 1;
}

ESP_EVENT_DECLARE_BASE(NEW_FRAME_POOL);

static SemaphoreHandle_t pool_mutex = NULL;

static inline size_t frame_size_bytes(void) {
    return capture_width * capture_height * BYTES_PER_PIXEL;
}

// Returns 1 if the frame was accepted into the pool, 0 if it was dropped.
// Caller MUST check this return value before advancing pattern state.
int push_frame_to_pool(camera_fb_t* fb, int format, int pattern_index) {
    (void)format;

    //frame = raw data from frame buffer
    uint8_t *frame = fb->buf;

    //assert variables are not null
    assert(frame != NULL);
    assert(frame_pool != NULL);
    assert(full_frame_pool != NULL);
    assert(frame_pool_indices != NULL);

    //size of frames

    size_t frame_size = frame_size_bytes();

    //Take sephamore (other functions wait for this to be returned, used for synchronization)
    xSemaphoreTake(pool_mutex, portMAX_DELAY);

    if (frames_in_pool >= frame_pool_size) {
        //before swapping, make sure that it can
        if(!can_frame_pool_swap) {
            xSemaphoreGive(pool_mutex);
            //ESP_LOGW(TAG, "Pool full, consumer not ready — dropping frame (pattern %d)", pattern_index);
            return 0;   // frame dropped — caller must not advance pattern index
        }

        //swap frame pools
        uint8_t *tmp = frame_pool;
        frame_pool = full_frame_pool;
        full_frame_pool = tmp;

        //swap frame pool indices
        uint8_t *tmp_idx = frame_pool_indices;
        frame_pool_indices = full_frame_pool_indices;
        full_frame_pool_indices = tmp_idx;

        frames_in_pool = 0;

        //create new finished frame pool struct
        finished_frame_pool finished_frame = {
            .data = full_frame_pool,
            .frame_count = frame_pool_size,
            .frame_width = capture_width,
            .frame_height = capture_height,
            .pattern_indices = full_frame_pool_indices   // NEW: tells consumer which slot is which pattern
        };

        can_frame_pool_swap = 0;

        //post event - new frame pool, pass on finished frame pool
        esp_event_post_to(event_loop_handle, NEW_FRAME_POOL, NEW_FRAME_POOL_ID, &finished_frame, sizeof(finished_frame), 10);
    }

    //copy frame to pool
    uint8_t *dst = frame_pool + (frames_in_pool * frame_size);
    memcpy(dst, frame, frame_size);

    frame_pool_indices[frames_in_pool] = (uint8_t) pattern_index;   // NEW: tag this slot

    // increase
    frames_in_pool++;

    //give sephamore, allows others to continue
    xSemaphoreGive(pool_mutex);

    return 1;   // frame accepted
}

void set_pool_size(int frame_count) {
    //ensure valid input
    if (frame_count <= 0) {
        ESP_LOGE(TAG, "Invalid frame count");
        return;
    }

    //number of bytes required
    size_t bytes =
        frame_size_bytes() *
        frame_count;

    //release frame pool if it is already allocated in memory
    if (frame_pool) {
        heap_caps_free(frame_pool);
        frame_pool = NULL;
    }

    // if full frame pool is full, release it
    if (full_frame_pool) {
        heap_caps_free(full_frame_pool);
        full_frame_pool = NULL;
    }

    //release memory from indices
    if (frame_pool_indices) {
        heap_caps_free(frame_pool_indices);
        frame_pool_indices = NULL;
    }

    //release memory from full frame pool indices
    if (full_frame_pool_indices) {
        heap_caps_free(full_frame_pool_indices);
        full_frame_pool_indices = NULL;
    }

    //allocate memory for frame pools
    frame_pool =
        heap_caps_malloc(
            bytes,
            MALLOC_CAP_SPIRAM);

    full_frame_pool =
        heap_caps_malloc(
            bytes,
            MALLOC_CAP_SPIRAM);

    //index arrays, one byte per frame slot — SPIRAM not required, they're tiny
    frame_pool_indices =
        heap_caps_malloc((size_t) frame_count, MALLOC_CAP_8BIT);

    full_frame_pool_indices =
        heap_caps_malloc((size_t) frame_count, MALLOC_CAP_8BIT);

    
    //check if created
    if (!frame_pool || !full_frame_pool || !frame_pool_indices || !full_frame_pool_indices) {
        ESP_LOGE(TAG, "Failed to allocate frame pools");

        if (frame_pool) { heap_caps_free(frame_pool); frame_pool = NULL; }
        if (full_frame_pool) { heap_caps_free(full_frame_pool); full_frame_pool = NULL; }
        if (frame_pool_indices) { heap_caps_free(frame_pool_indices); frame_pool_indices = NULL; }
        if (full_frame_pool_indices) { heap_caps_free(full_frame_pool_indices); full_frame_pool_indices = NULL; }

        return;
    }

    frame_pool_size = frame_count;
    frames_in_pool = 0;

    ESP_LOGI(TAG,
             "Allocated frame pools (%u bytes each)",
             (unsigned)bytes);
}

esp_err_t init_frame_pool(void) {
    //create mutex
    pool_mutex = xSemaphoreCreateMutex();

    if (pool_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }
    
    set_pool_size(1);

    return ESP_OK;
}
