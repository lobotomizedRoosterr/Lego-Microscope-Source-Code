#include "esp_lcd_panel_io.h"
#include "esp_lcd_ili9341.h"
#include "esp_event.h"

enum {
    NEW_FRAME_POOL_ID,
    NEW_FRAME_POOL_PROCESSED_ID
};

extern esp_lcd_panel_handle_t lcd_panel_handle;
extern esp_event_loop_handle_t event_loop_handle;
