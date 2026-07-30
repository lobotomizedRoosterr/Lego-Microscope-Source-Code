#include "esp_lcd_panel_io.h"
#include "esp_lcd_ili9341.h"
#include "esp_event.h"

/*
 * These define IDs for two events
*/
enum {
    NEW_FRAME_POOL_ID, /*Event ID called for when a frame pool has been finished, and has been swapped. Signals that image processing can begin*/
    NEW_FRAME_POOL_PROCESSED_ID /*Event called for when image processing has completed. Lets frame pool swaps continue*/
};

/*
 * identifier for lcd panel. Used with esp-lcd driver
*/
extern esp_lcd_panel_handle_t lcd_panel_handle;
/*
 * handle for the esp_event_loop. Used with esp_event.
*/
extern esp_event_loop_handle_t event_loop_handle;
