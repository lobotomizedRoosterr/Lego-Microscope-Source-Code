#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_lcd_ili9341.h"

esp_err_t init_render_engine();

extern uint16_t* frame_buffer;

extern SemaphoreHandle_t frame_done_sem;

void push_frame();

void push_buffer_to_frame(uint16_t* img_buf);

void greyscale_to_rgb565(uint8_t *greyscale, uint16_t *rgb565, int width, int height);

uint16_t color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

void draw_pixel(int x, int y, uint16_t color);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_image(
    int dst_x,
    int dst_y,
    const uint8_t *img,
    int width,
    int height
);
void draw_char(
    int x,
    int y,
    char c,
    uint16_t color);
void draw_rect(int x, int y, int w, int h, uint16_t color);
void fill_rect(int x, int y, int w, int h, uint16_t color);
void draw_text(int x, int y, const char *str, uint16_t color);
void lcd_draw_debug_bitmap(esp_lcd_panel_handle_t panel,
                           int x,
                           int y,
                           int width,
                           int height);