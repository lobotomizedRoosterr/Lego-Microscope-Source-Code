#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_event.h"


#include "main/app_state.h"

#include "components/include/spi_bus_manager.h"
#include "components/include/display.h"
#include "components/include/camera.h"
#include "components/include/mode_registry.h"
#include "components/include/frame_pool.h"
#include "components/include/acquisition_sequencer.h"
#include "components/include/led_matrix.h"
#include "computation/include/computation.h"
#include "components/include/render_engine.h"
#include "components/include/ui.h"
#include "components/include/touch.h"
#include "esp_lcd_panel_ops.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "components/include/image_store.h"

static const char* TAG = "MAIN"; 

esp_event_loop_handle_t event_loop_handle;

//I might wanna change the name of some of this stuff it feels weird idk

ESP_EVENT_DECLARE_BASE(NEW_FRAME_POOL);
ESP_EVENT_DEFINE_BASE(NEW_FRAME_POOL);

ESP_EVENT_DECLARE_BASE(NEW_FRAME_POOL_PROCESSED);
ESP_EVENT_DEFINE_BASE(NEW_FRAME_POOL_PROCESSED);

/*
 * This initializes the event system.
 * The event system is used for two events (as of now)
 * - New Frame Pool = frame pool completed, image processing can begin
 * - New Frame Pool Processed = image processing can begin, allow swapping of frame pool when full
 *    This disallows swapping a frame pool during image processing. So if a new frame pool is ready when image processing 
 *    is not, the frame pools will not be swapped unless this event has been called
*/
esp_err_t init_event_system(void) {
    esp_err_t ret = ESP_OK;

    esp_event_loop_args_t loop_args = {
        .queue_size = 2, // max event calls in queue
        .task_name = "event loop", // name for loop
        .task_priority = 1, // priority for tasks
        .task_stack_size = 4096, // byte stack size
        .task_core_id = 1 // core to use
    };

    //Just used to associate the event with a given structure of data.
    finished_frame_pool fp = {
        .data = NULL,
        .frame_count = 0,
        .frame_width = 0,
        .frame_height = 0
    };
    // create event loop
    ret = esp_event_loop_create(&loop_args, &event_loop_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_register_with(event_loop_handle, NEW_FRAME_POOL, NEW_FRAME_POOL_ID, begin_new_computation, &fp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register frame pool handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_register_with(event_loop_handle, NEW_FRAME_POOL_PROCESSED, NEW_FRAME_POOL_PROCESSED_ID, new_frame_pool_processed, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register processed handler: %s", esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

/*
 * Simplifies intializing components. Each component uses a similar structure for function calling.
 * @param component_init component to initialize
 * @param log_descriptor tag for the initialization, what to print (ex. initializing "log_descriptor")
*/
void init_standard_component(esp_err_t(component_init)(), const char* log_descriptor) {
    ESP_LOGI(TAG, "Initializing component %s", log_descriptor);
    esp_err_t err = component_init();
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize component %s | %s", log_descriptor, esp_err_to_name(err));
        esp_restart();
    }
    ESP_LOGI(TAG, "%s initialized", log_descriptor);
}
// Main function called on boot
void app_main(void) {
    //prints reasons the system rebooted. Great for debug.
    ESP_LOGI(TAG, "Reset reason: %d", esp_reset_reason());

    ESP_LOGI(TAG, "Starting Microscope...");

    //This runs every components initialization function.
    init_standard_component(init_spi_bus_manager, (char*)"spi bus manager");
    init_standard_component(init_display, (char*)"display");
    init_standard_component(init_render_engine, (char*)"render engine");
    init_standard_component(init_camera, (char*)"camera");
    init_standard_component(init_led_matrix, (char*)"led array");
    init_standard_component(init_touch, (char*)"touch");
    init_standard_component(init_mode_registry, (char*)"mode registry");
    init_standard_component(init_frame_pool, (char*)"frame pool");
    init_standard_component(init_acquisition_sequencer, "acquisition_sequencer");
    init_standard_component(init_computation, "computation");
    init_standard_component(init_event_system, "event system");
    init_standard_component(image_store_init, "image storage");
    //init_standard_component(screen_manager_init, "ui");
    
    xTaskCreatePinnedToCore(
        ui_task,
        "ui_task",
        4092,
        NULL,
        1,
        NULL,
        0
    );
    /* This pins the camera acquisition loop to core1.*/
    xTaskCreatePinnedToCore(
        camera_acquisition_task,
        "camera_acquisition",
        8192,
        NULL,
        3,
        NULL,
        1
    );
}
