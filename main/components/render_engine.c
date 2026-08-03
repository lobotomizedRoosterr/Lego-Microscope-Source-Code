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
#include "resources/fonts/font_8x14.h"
#include "resources/fonts/font_8x8.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdlib.h>
#include <string.h>

/*
 * If there is no notes on a variable, check the header. 
*/

#define FRAME_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint32_t)) /*size of framebuffer, set to width * height * bytes in a 18 bit pixel (2)*/

/*
 * These are different color orders lcds might use. This needs to be found out (often), because cheap lcds often 
 * use different orders than normal.
*/
#define COLOR_ORDER_RGB 0 /*Red Green Blue*/
#define COLOR_ORDER_RBG 1 /* Red Blue Green*/
#define COLOR_ORDER_BGR 2 /*Blue Green Red*/

#define COLOR_TRANSPARENT (rgb666_color_t) {63,0,63}

#define COLOR_ORDER COLOR_ORDER_RBG /*color order of display*/

static const char* TAG = "RENDER_ENGINE";


uint8_t* frame_buffer = NULL;


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

void draw_greyscale_image(uint8_t *greyscale, int x, int y, int width, int height) {
    for(int dst_x = 0; dst_x < width; dst_x++) {
        for(int dst_y = 0; dst_y < height; dst_y++) {
            size_t grey_idx = (size_t) dst_y*width+dst_x;

            uint8_t grey = greyscale[grey_idx]/(255/63);

            uint8_t grey6 = grey << 2;

            if(grey6 > 63) grey6=63;

            draw_pixel(x+dst_x, y+dst_y, (rgb666_color_t){grey6,grey6,grey6});
        }
    }
}

void draw_pixel(int x, int y, rgb666_color_t color) {
    // Ensure it is within bounds
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    
    // Calculate the 1D index in the 32-bit frame buffer
    int index = (y * DISPLAY_WIDTH + x) * 3;
    
    // Convert 6-bit channels to 8-bit by shifting left by 2 (scales 0-63 to 0-252)
    uint8_t r8 = (uint32_t)(color.r & 0x3F) << 2;
    uint8_t g8 = (uint32_t)(color.g & 0x3F) << 2;
    uint8_t b8 = (uint32_t)(color.b & 0x3F) << 2;

    frame_buffer[index] = r8;
    frame_buffer[index+1] = g8;
    frame_buffer[index+2] = b8;

}

/*
 * This fills a rectangle.
 * @param x x location of rectangle
 * @param y y location of rectangle
 * @param w width of rectangle
 * @param h height of rectangle
 * @param color rgb565 color of rectangle
*/
void fill_rect_bounds(int x0, int y0, int x1, int y1, rgb666_color_t color) {
    fill_rect(x0, y0, x1-x0, y1-y0, color);
}

/*
 * This fills a rectangle.
 * @param x x location of rectangle
 * @param y y location of rectangle
 * @param w width of rectangle
 * @param h height of rectangle
 * @param color rgb565 color of rectangle
*/
void draw_rect_bounds(int x0, int y0, int x1, int y1, rgb666_color_t color) {
    draw_rect(x0, y0, x1-x0, y1-y0, color);
}

void draw_line(int x0, int y0, int x1, int y1, rgb666_color_t color) {

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
void draw_char(int x, int y, char c, rgb666_color_t color, int font) {
    // make sure defined character exists in the font we use.
    if (c < 32 || c > 127)
        return;
    // the glyph is just 14 8 bit integers, every pixel of the defined character is a bit.
    const uint8_t *glyph = font8x14[c - 32];

    // go through each row
    for (int row = 0; row < 14; row++) {
        // 8 bit integer, row definition
        uint8_t bits = glyph[row];

        for (int col = 0; col < 8; col++) {
        // write color at location writes if that bit is 1, not 0
            if (bits & (0x80 >> col)) {
                draw_pixel(x+col, y+row, color);
            }
        }
    }
}
rgb666_color_t color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {

    rgb666_color_t ret;
    ret.r=r >> 2;
    ret.g=g >> 2;
    ret.b=b >> 2;
    return ret;
}
void draw_rect(int x, int y, int w, int h, rgb666_color_t color) {
    if (w <= 0 || h <= 0)
        return;

    // draw borders of the rectangle
    draw_line(x, y, x + w - 1, y, color);
    draw_line(x, y, x, y + h - 1, color);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    draw_line(x,y + h - 1, x + w - 1, y + h - 1, color);
}
void fill_rect(int x, int y, int w, int h, rgb666_color_t color) {
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
void draw_text(int x, int y, const char *str, rgb666_color_t color, int font) {
    while (*str) {
        //draw character at the x value, which increases based on spacing
        draw_char(x, y, *str, color, font);

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
        (const void*) frame_buffer);

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
           uint8_t r = img[index]*63/255;
           uint8_t g = img[index+1]*63/255;
           uint8_t b = img[index+2]*63/255;

           if((r!=COLOR_TRANSPARENT.r) | (g!=COLOR_TRANSPARENT.g) |( b!=COLOR_TRANSPARENT.b)) {
                draw_pixel(x+dst_x, y+dst_y, (rgb666_color_t){b,g,r});
           };



            //draw pixel at location
            //draw_pixel(x + dst_x, dst_y + y, pixel_rgb);
            //increment by 2, as each pixel uses 2 integers, not one
            index += 3;
        }
    }
}
