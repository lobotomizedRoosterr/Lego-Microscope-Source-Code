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

#include <math.h>
#include "esp_dsp.h"
#include "esp_random.h"

static const char* TAG = "COMPUTATION";

ESP_EVENT_DECLARE_BASE(NEW_FRAME_POOL_PROCESSED);

size_t frame_size = (size_t) 0;

float c_scale = 0.9f; // Default scaling factor for QDF computation

float* computation_frame;

int frames;

/*
Think about having every pixel value being a float,
then placing each pixel as an int value from 0 to 255,
but normalizing them based on range.
(perhaps highest float value becomes 255, lowest becomes 0, etc.)
Or something like that
*/

/* ---------------------------------------------------------------------
 * Scratch buffers, allocated ONCE in init_computation() and reused every
 * frame. Modes never run concurrently (current_mode picks exactly one
 * path per frame), so QDF (needs 4 converted sources + 2 working buffers)
 * and DPC (needs 2 converted sources + 2 working buffers) can safely
 * alias the same 6-buffer pool. This avoids heap_caps_malloc/free churn
 * on the real-time acquisition path.
 *
 * NOTE ON WHAT esp-dsp ACTUALLY COVERS HERE:
 * esp-dsp's basic math ops (dsps_add_f32 / dsps_sub_f32 / dsps_addc_f32 /
 * dsps_mulc_f32) operate on float arrays and have ESP32-S3-optimized
 * (_aes3) assembly backends selected automatically at build time. They do
 * NOT include an elementwise vector abs() or elementwise divide - those
 * remain scalar loops below (see comments at each call site).
 * --------------------------------------------------------------------- */
static float* scratch[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

// Aliases used by QDF (4 converted sources + 2 working buffers)
#define qdf_tl_f  scratch[0]
#define qdf_tr_f  scratch[1]
#define qdf_br_f  scratch[2]
#define qdf_bl_f  scratch[3]
#define qdf_df    scratch[4]   // holds DF, then c_scale*DF
#define qdf_e     scratch[5]   // holds E

// Aliases used by DPC (2 converted sources + numerator/denominator)
#define dpc_a_f   scratch[0]
#define dpc_b_f   scratch[1]
#define dpc_num   scratch[4]
#define dpc_den   scratch[5]

static inline void cast_u8_to_f32(const uint8_t* src, float* dst, size_t n) {
    // Straight contiguous cast - no esp-dsp function exists for u8->f32
    // conversion, but a single linear pass here (vs. the original's
    // strided x-outer/y-inner indexing) is itself the main cache win for
    // BF/DF, since frame data is stored row-major.
    for (size_t i = 0; i < n; i++) {
        dst[i] = (float) src[i];
    }
}

void begin_new_computation(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
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
    //greyscale_to_rgb565(computation_frame, frame_buffer, capture_width, capture_height);
    draw_value_map(&computation_frame, 0, 0, 320, 240);

    esp_event_post_to(event_loop_handle, NEW_FRAME_POOL_PROCESSED, NEW_FRAME_POOL_PROCESSED_ID, NULL, 0, 10);
}

void run_bf(uint8_t** data, float** buf) {
    cast_u8_to_f32(*data, *buf, frame_size);
}

void run_df(uint8_t** data, float** buf) {
    cast_u8_to_f32(*data, *buf, frame_size);
}

void run_qdf(uint8_t** data, float** buf, float c_scale) {
    //TODO: Look at led orientation stuff.

    const uint8_t* tl_img = *data;
    const uint8_t* tr_img = *data + frame_size;
    const uint8_t* br_img = *data + frame_size * 2;
    const uint8_t* bl_img = *data + frame_size * 3;

    const int len = (int) frame_size;

    // Convert each quadrant to float once. Unavoidably scalar (no esp-dsp
    // u8->f32 conversion), but contiguous so it's memory-bandwidth-bound
    // rather than cache-thrashing like the original x-outer/y-inner loop.
    cast_u8_to_f32(tl_img, qdf_tl_f, frame_size);
    cast_u8_to_f32(tr_img, qdf_tr_f, frame_size);
    cast_u8_to_f32(br_img, qdf_br_f, frame_size);
    cast_u8_to_f32(bl_img, qdf_bl_f, frame_size);

    // --- Darkfield image: DF = tl + tr + br + bl (Eq. 2), via SIMD-backed adds ---
    dsps_add_f32(qdf_tl_f, qdf_tr_f, qdf_df, len, 1, 1, 1); // qdf_df = tl + tr
    dsps_add_f32(qdf_br_f, qdf_bl_f, qdf_e,  len, 1, 1, 1); // qdf_e (reused) = br + bl
    dsps_add_f32(qdf_df, qdf_e, qdf_df, len, 1, 1, 1);      // qdf_df = DF

    // Scale DF by c_scale in place
    dsps_mulc_f32(qdf_df, qdf_df, len, c_scale, 1, 1);      // qdf_df = c_scale * DF

    // --- Edge image: E = |tl-br| + |bl-tr| (Eq. 1) ---
    // esp-dsp has no elementwise vector abs, so this stays a scalar loop.
    for (size_t i = 0; i < frame_size; i++) {
        qdf_e[i] = fabsf(qdf_tl_f[i] - qdf_br_f[i]) + fabsf(qdf_bl_f[i] - qdf_tr_f[i]);
    }

    // --- QDF = c_scale*DF - E ---
    dsps_sub_f32(qdf_df, qdf_e, *buf, len, 1, 1, 1);
}

// Shared DPC core: out = (a - b) / (a + b + eps)
// Numerator/denominator prep is vectorized via esp-dsp; the division
// itself stays scalar since esp-dsp has no elementwise vector divide.
static void run_dpc_common(const uint8_t* a_img, const uint8_t* b_img, float* out) {
    const int len = (int) frame_size;

    cast_u8_to_f32(a_img, dpc_a_f, frame_size);
    cast_u8_to_f32(b_img, dpc_b_f, frame_size);

    dsps_sub_f32(dpc_a_f, dpc_b_f, dpc_num, len, 1, 1, 1);       // num = a - b
    dsps_add_f32(dpc_a_f, dpc_b_f, dpc_den, len, 1, 1, 1);       // den = a + b
    dsps_addc_f32(dpc_den, dpc_den, len, 0.0001f, 1, 1);         // den += eps

    // DPC values range between -1 and 1.
    for (size_t i = 0; i < frame_size; i++) {
        out[i] = (dpc_num[i] / dpc_den[i] + 1.0f) / 2.0f * 255.0f;
    }
}

void run_dpc_lr(uint8_t** data, float** buf) {
    const uint8_t* l_img = *data;
    const uint8_t* r_img = *data + frame_size;
    run_dpc_common(l_img, r_img, *buf); // (l - r) / (l + r + eps)
}

void run_dpc_rl(uint8_t** data, float** buf) {
    const uint8_t* l_img = *data;
    const uint8_t* r_img = *data + frame_size;
    run_dpc_common(r_img, l_img, *buf); // (r - l) / (r + l + eps)
}
//when well is full, record max value. Something weird in math, intensity too high.

void run_dpc_tb(uint8_t** data, float** buf) {
    const uint8_t* t_img = *data;
    const uint8_t* b_img = *data + frame_size;
    run_dpc_common(t_img, b_img, *buf); // (t - b) / (t + b + eps)
}

void run_dpc_bt(uint8_t** data, float** buf) {
    const uint8_t* t_img = *data;
    const uint8_t* b_img = *data + frame_size;
    run_dpc_common(b_img, t_img, *buf); // (b - t) / (b + t + eps)
}

esp_err_t init_computation() {
    esp_err_t ret = ESP_OK;

    frame_size = (size_t) capture_width*capture_height;

    //allocate a buffer for the computation frame.
    computation_frame = heap_caps_malloc(frame_size * sizeof(float), MALLOC_CAP_SPIRAM);
    if (computation_frame == NULL) {
        ESP_LOGE(TAG, "Failed to allocate initial computation_frame");
        return ESP_ERR_NO_MEM;
    }

    // Allocate the shared scratch pool used by run_qdf / run_dpc_common.
    // Sized once here so no mode does a heap_caps_malloc/free per frame.
    for (int i = 0; i < 6; i++) {
        scratch[i] = heap_caps_malloc(frame_size * sizeof(float), MALLOC_CAP_SPIRAM);
        if (scratch[i] == NULL) {
            ESP_LOGE(TAG, "Failed to allocate computation scratch buffer %d", i);
            return ESP_ERR_NO_MEM;
        }
    }

    return ret;
}