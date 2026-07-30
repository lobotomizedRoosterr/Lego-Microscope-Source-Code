#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_lcd_ili9341.h"


/*
 * Notes
 * Every draw function's position is not centered. i.e. (x, y) defines a corner of the object. For example, centering a rectangle would see 
 * its (x, y) as (x+width/2, y+height/2), if the rectangle were to be centered about a point.
 * 
 * Every draw function does not draw directly to the screen. It configures the frame buffer, which is pushed to the screen.
*/

/*initialize render engine*/
esp_err_t init_render_engine();

/*this single buffer contains the data that will be pushed to the display over the spi2 bus
* whenever push_frame() is called
*/
extern uint16_t* frame_buffer;


/*
 * push the frame buffer to the lcd
*/
void push_frame();

/*
 * Converts from greyscale 1 byte format to RGB565 2 byte format
 * @param greyscale this is the greyscale buffer to be converted into RGB565
 * @param rgb565 this is where the converted buffer will be applied
 * @param width width of image
 * @param height height of image
*/
void greyscale_to_rgb565(uint8_t *greyscale, uint16_t *rgb565, int width, int height);

/*
 * this converts 3 8 bit r g and blue integers into a single 16 bit rgb565 It also takes into account specified color order
 * @param r red value 0-255
 * @param g green value 0-255
 * @param b blue value 0-255
*/
uint16_t color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

/*
 * This sets a specific pixel of the frame buffer to a specific color value.
 * @param x x value to set
 * @param y y value to set
 * @param color rgb565 value
*/
void draw_pixel(int x, int y, uint16_t color);
/*
 * This draws a line between two points. 
 * @param x0 first point x value
 * @param y0 first point y value
 * @param x1 second point x value
 * @param y1 secont point y value
 * @param color rgb565 color value
*/
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
/*
 * This draws images. Note that while img is of 8 bit integers, each pixel is represented by two 8 bit integers, and these integers are combined 
 * during rendering to create a single 16 bit integer. Therefore, images are still rgb565 and 16 bit. This is done because the LVGL tool to convert images into 
 * C arrays does it this way, and it was easier to configure everything like this.
 * @param x x location of image
 * @param y y location of image
 * @param img raw data for image. Note that while it is of 8 bit integers, it still represents 16 bit color data, as each pixel uses 2 8 bit integers.
 * @param width width of image
 * @param height height of image
 * 
*/
void draw_image(
    int x,
    int y,
    const uint8_t *img,
    int width,
    int height
);
/*
 * This draws a character at a specific location. Note that there is currently only 1 font, and that font is 8x14 pixels.
 * @param x x value to draw char
 * @param y y value to draw char
 * @param c character to draw
 * @param color rgb565 color to draw
*/
void draw_char(
    int x,
    int y,
    char c,
    uint16_t color);
/*
 * This draws a rectangle. It draws border, to fill one, use fill rect.
 * @param x x value to draw rectangle
 * @param y y value to draw rectangle
 * @param w width of rectangle
 * @param h height of rectangle
 * @param color rgb565 color of rectangle
*/
void draw_rect(int x, int y, int w, int h, uint16_t color);
/*
 * This fills a rectangle.
 * @param x x location of rectangle
 * @param y y location of rectangle
 * @param w width of rectangle
 * @param h height of rectangle
 * @param color rgb565 color of rectangle
*/
void fill_rect(int x, int y, int w, int h, uint16_t color);
/*
 * This draws a string of characters. Note that the only font is 8x14. every space is a single pixel. Keep this in mind when designing UIs.
 * @param x x value to draw string
 * @param y y value to draw string
 * @param str characters to draw
 * @param color rgb565 color to draw string in
*/
void draw_text(int x, int y, const char *str, uint16_t color);
