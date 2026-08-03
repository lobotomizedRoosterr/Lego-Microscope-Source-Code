#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_io_spi.h"

#define DISPLAY_WIDTH 320 /*Width of display in px*/
#define DISPLAY_HEIGHT 240 /*height of display in px*/


/* handle used to identify the io configuration of the lcd*/
extern esp_lcd_panel_io_handle_t lcd_io_handle;

/*initializes display*/
esp_err_t init_display();
