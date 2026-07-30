#include "components/include/acquisition_sequencer.h"
#include "components/include/camera.h"
#include "components/include/frame_pool.h"
#include "components/include/led_matrix.h"
#include "main/app_state.h"
#include "components/include/render_engine.h"
#include "components/include/mode_registry.h"
#include "computation/include/computation.h"
#include "main/pin_config.h"
#include "esp_err.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_event.h"
#include "components/include/ui.h"
#include "freertos/FreeRTOS.h"

#define BG_IMG_NUM 4

microscopy_mode mode;

int current_pattern_index = 0;

void camera_acquisition_task(void *arg) {

    mode = get_mode_from_id(current_mode);

    uint8_t current_pattern[8] = {0};
    int last_sent_pattern_index = -1; // force initial pattern send + flush

    while(1) {
        //detect change in mode
        if(get_mode_from_id(current_mode).label != mode.label) {
            mode = get_mode_from_id(current_mode);
            current_pattern_index = 0;
            last_sent_pattern_index = -1; // force resend + flush on mode change

            set_pool_size(mode.num_patterns);
        }
        // cap pattern index
        if(current_pattern_index >= mode.num_patterns) {
            current_pattern_index = 0;
        }

        //detect pattern change
        bool pattern_changed = (current_pattern_index != last_sent_pattern_index);
        //set led array intensity
        led_array_set_intensity(mode.led_brightness_val);
        if (pattern_changed) {
            // get pattern
            get_pattern_from_index(current_pattern_index, current_pattern, &mode);
            //set pattern
            led_array_send_matrix(current_pattern);
            vTaskDelay(pdMS_TO_TICKS(mode.exposure_time)); // LED + AEC/AGC settle

            // Flush one frame that may have begun integrating under the
            // PREVIOUS pattern (esp32-camera's DMA double-buffering can have
            // a frame already in flight when we switch). This guarantees the
            // next fb_get() below is a frame whose entire rolling readout
            // happened entirely under the pattern we just set -- i.e.
            // implicit global shutter w.r.t. illumination.
            camera_fb_t *stale_fb = camera_capture_frame();
            if (stale_fb) {
                camera_return_frame(stale_fb);
            }

            last_sent_pattern_index = current_pattern_index;
        }

        camera_fb_t* fb = camera_capture_frame();

        // Only advance pattern index if the frame was actually accepted into
        // the pool. If dropped (pool full / consumer not ready), retry the
        // SAME pattern next loop -- and since current_pattern_index is
        // unchanged, pattern_changed will be false above, so we correctly
        // skip the resend/flush and just grab another frame under the
        // illumination that's already correctly set.
        int accepted = push_frame_to_pool(fb, PIXFORMAT_GRAYSCALE, current_pattern_index);
        if (accepted) {
            current_pattern_index++;
        }

        update_ui();
        render_ui();

        push_frame();

        camera_return_frame(fb);
    }
}
esp_err_t init_acquisition_sequencer() {
    esp_err_t ret = ESP_OK;

    return ret;
}
