#include "components/include/render_engine.h"
#include "components/include/display.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "main/app_state.h"
#include "components/include/spi_bus_manager.h"
#include "components/include/ui.h"
#include "main/pin_config.h"
#include "components/include/font_8x14.h"

#include <stdlib.h>
#include <string.h>

/*
 * If there is no notes on a variable, check the header. 
*/

#define FRAME_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t)) /*size of framebuffer, set to width * height * bytes in a 16 bit pixel (2)*/

/*
 * These are different color orders lcds might use. This needs to be found out (often), because cheap lcds often 
 * use different orders than normal.
*/
#define COLOR_ORDER_RGB 0 /*Red Green Blue*/
#define COLOR_ORDER_RBG 1 /* Red Blue Green*/
#define COLOR_ORDER_BGR 2 /*Blue Green Red*/

#define COLOR_ORDER COLOR_ORDER_RBG /*color order of display*/

static const char* TAG = "RENDER_ENGINE";


uint16_t* frame_buffer = NULL;

esp_err_t init_render_engine() {
    esp_err_t ret = ESP_OK;

    //allocate space in psram for the frame buffer
    frame_buffer = heap_caps_malloc(FRAME_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);

    //check if it could be created
    if (frame_buffer == NULL) {
        ESP_LOGE(TAG,
                 "Failed to allocate framebuffer (%d bytes)",
                 FRAME_BUFFER_SIZE);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG,
             "frame_buffer allocated at %p",
             frame_buffer);

    return ret;
}



uint16_t rgb565_from_greyscale(uint8_t grey) {
    uint16_t r = (grey >> 3);
    uint16_t g = (grey >> 2);
    uint16_t b = (grey >> 3);

    uint16_t val = (r << 11) | (g << 5) | b;

    uint16_t rgb565 = (val >> 8) | (val << 8);

    return rgb565;
}

void greyscale_to_rgb565(uint8_t *greyscale, uint16_t *rgb565, int width, int height) {

    int pixels = width * height;

    if (greyscale == NULL) {
        ESP_LOGE(TAG, "gray buffer is NULL");
        return;
    }

    if (rgb565 == NULL) {
        ESP_LOGE(TAG, "rgb565 buffer is NULL");
        return;
    }

    for (size_t i = 0; i < pixels; i++) {
        // get grey value (8 bit integer)
        uint8_t grey = greyscale[i];

        //set r g and b values to this grey value. (all are the same for grey)
        uint16_t r = (grey >> 3); // 5 bits
        uint16_t g = (grey >> 2); // 6 bits
        uint16_t b = (grey >> 3); // 5 bits

        //shift bytes for 16 bit color format
        uint16_t val = (r << 11) | (g << 5) | b;
        //configure rgb565 buffer given
        rgb565[i] = (val >> 8) | (val << 8);
    }
}


void draw_value_map(float **map, int x, int y, int width, int height) {
    for(int dx = 0; dx < width; dx++) {
        for(int dy = 0; dy < height; dy++) {
            size_t idx = (size_t) dy * width + dx;
            float val_f = (*map)[idx];
            //convert to 8 bit integer value
            uint8_t val = (uint8_t) val_f;

            if(val > 255) val = 255;
            if(val < 0) val = 0;

            //convert to 16 bit integer
            draw_pixel(x + dx, y + dy, rgb565_from_greyscale(val));
        }
    }
}

void draw_pixel(int x, int y, uint16_t color) {
    //ensure it is within bounds
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    //set frame buffer position to the color
    frame_buffer[y * DISPLAY_WIDTH + x] = color;
}
void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {

    /*
     * This is just Bresenham's line drawing algorithm
    */

    int dx = abs(x1 - x0); // horizontal distance between point 1 and 2
    int dy = abs(y1 - y0); // vertical distance between point 1 and 2

    int sx = (x0 < x1) ? 1 : -1; // Step direction for x-axis

    int sy = (y0 < y1) ? 1 : -1; // step direction for y-axis

    int err = dx - dy; // error value used to determine when to move up and when to move down

    while (1) {
        //draw pixel at location
        draw_pixel(x0, y0, color);

        //end if we are at the desired endpoint
        if (x0 == x1 && y0 == y1)
            break;


        int e2 = 2 * err; // used to decide whether to move in x direction, y direction, or both

        //should line move along x?
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        //should line move along y?
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}
void draw_char(int x, int y, char c, uint16_t color) {
    // make sure defined character exists in the font we use.
    if (c < 32 || c > 127)
        return;
    // the glyph is just 14 8 bit integers, every pixel of the defined character is a bit.
    const uint8_t *glyph = font8x14_basic[c - 32];

    // go through each row
    for (int row = 0; row < 14; row++) {
        // 8 bit integer, row definition
        uint8_t bits = glyph[row];

        for (int col = 0; col < 8; col++) {
        // write color at location writes if that bit is 1, not 0
            if (bits & (0x80 >> col)) {
                frame_buffer[(y + row) * DISPLAY_WIDTH + (x + col)] = color;
            }
        }
    }
}
uint16_t color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // these are first, second, and third pieces for the color
    uint8_t c0, c1, c2;
    
    // configures color values based on color order. (c0 (first) = R for RGB, but B for BGR)
    switch(COLOR_ORDER) {
        case COLOR_ORDER_RGB:
            c0=r;
            c1=g;
            c2=b;
            break;
        case COLOR_ORDER_RBG:
            c0=r;
            c1=b;
            c2=g;
            break;
        case COLOR_ORDER_BGR:
            c0=b;
            c1=g;
            c2=r;
            break;
    }
    // This seems to work fine, but look into if the middle value is always 6 bits even if it doesn't represent green.
    uint16_t r5 = (c0 >> 3) & 0x1F; // 5 bits
    uint16_t g6 = (c1 >> 2) & 0x3F; // 6 bits
    uint16_t b5 = (c2 >> 3) & 0x1F; // 5 bits

    return (r5 << 11) | (g6 << 5) | b5;
}
void draw_rect(int x, int y, int w, int h, uint16_t color) {
    if (w <= 0 || h <= 0)
        return;

    // draw borders of the rectangle
    draw_line(x, y, x + w - 1, y, color);
    draw_line(x, y, x, y + h - 1, color);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    draw_line(x,y + h - 1, x + w - 1, y + h - 1, color);
}
void fill_rect(int x, int y, int w, int h, uint16_t color) {
    //ensure proper values for width
    if (w <= 0 || h <= 0)
        return;

    //draw lines, one line for every row of the rectangle
    for (int iy = 0; iy < h; iy++) {
        draw_line(x, y + iy,
                      x + w - 1, y + iy,
                      color);
    }
}
/*
 * This function seems to modify parameters, this should be fixed
*/
void draw_text(int x, int y, const char *str, uint16_t color) {
    while (*str) {
        //draw character at the x value, which increases based on spacing
        draw_char(x, y, *str, color);

        x += 9; // 8 pixels + 1 spacing
        str++;
    }
}

void push_frame(void) {
    //ensure frame is defined
    if (frame_buffer == NULL) {
        ESP_LOGE(TAG, "frame_buffer is NULL");
        return;
    }

    // use esp-lcd driver to push frame buffer to display
    esp_err_t ret = esp_lcd_panel_draw_bitmap(
        lcd_panel_handle,
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        frame_buffer);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to push frame: %s",
                 esp_err_to_name(ret));
    }
}
void draw_image( int x, int y, const uint8_t *img, int width, int height) {
    if (img == NULL) {
        return;
    }
    int index = 0;

    // loop through every single pixel of the image
    for (int dst_y = 0; dst_y < height; dst_y++) {
        for (int dst_x = 0; dst_x < width; dst_x++) {
            /*
            * Each picture is 16 bit, but stored in 8 bit integers, such that every pixel uses 2 integers
            */
            uint16_t pixel_rgb = img[index] << 8 | img[index + 1];

            //draw pixel at location
            draw_pixel(x + dst_x, dst_y + y, pixel_rgb);
            //increment by 2, as each pixel uses 2 integers, not one
            index += 2;
        }
    }
}
