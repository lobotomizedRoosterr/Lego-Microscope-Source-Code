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

#define FRAME_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t))

#define COLOR_ORDER_RGB 0
#define COLOR_ORDER_RBG 1
#define COLOR_ORDER_BGR 2

#define COLOR_ORDER COLOR_ORDER_RBG

static const char* TAG = "RENDER_ENGINE";

uint16_t* frame_buffer = NULL;

SemaphoreHandle_t frame_done_sem = NULL;

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

void lcd_draw_debug_bitmap(esp_lcd_panel_handle_t panel,
                           int x,
                           int y,
                           int width,
                           int height)
{
    uint16_t *framebuffer = heap_caps_malloc(
        width * height * sizeof(uint16_t),
        MALLOC_CAP_DMA);

    if (!framebuffer)
        return;

    // 8 vertical color bars
    const uint16_t colors[] = {
        color_from_rgb(255,255,255),   // White
        color_from_rgb(255,255,0),     // Yellow
        color_from_rgb(0,255,255),     // Cyan
        color_from_rgb(0,255,0),       // Green
        color_from_rgb(255,0,255),     // Magenta
        color_from_rgb(255,0,0),       // Red
        color_from_rgb(0,0,255),       // Blue
        color_from_rgb(0,0,0)          // Black
    };


    for (int yy = 0; yy < height; yy++)
    {
        for (int xx = 0; xx < width; xx++)
        {
            int bar = (xx * 8) / width;
            framebuffer[yy * width + xx] = colors[bar];

            if(yy>200) {
                float val_f = ((float) yy-200.0f)/40.0f*255.0f;
                int val = (int) val_f;
                framebuffer[yy * width + xx] = color_from_rgb(val, val, val);
            }
        }
    }


    esp_lcd_panel_draw_bitmap(
        panel,
        x,
        y,
        x + width,
        y + height,
        framebuffer);

    free(framebuffer);
}


esp_err_t init_render_engine() {
    esp_err_t ret = ESP_OK;

    frame_buffer = heap_caps_malloc(FRAME_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);

    if (frame_buffer == NULL) {
        ESP_LOGE(TAG,
                 "Failed to allocate framebuffer (%d bytes)",
                 FRAME_BUFFER_SIZE);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG,
             "frame_buffer allocated at %p",
             frame_buffer);

    frame_done_sem = xSemaphoreCreateBinary();
    if (frame_done_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create frame transfer semaphore");
        return ESP_ERR_NO_MEM;
    }

    return ret;
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
        uint8_t grey = greyscale[i];

        uint16_t r = (grey >> 3); // 5 bits
        uint16_t g = (grey >> 2); // 6 bits
        uint16_t b = (grey >> 3); // 5 bits

        uint16_t val = (r << 11) | (g << 5) | b;
        rgb565[i] = (val >> 8) | (val << 8);
    }
}

void draw_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }

    frame_buffer[y * DISPLAY_WIDTH + x] = color;
}
void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (1)
    {
        draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}
void draw_char(
    int x,
    int y,
    char c,
    uint16_t color)
{
    if (c < 32 || c > 127)
        return;

    const uint8_t *glyph = font8x14_basic[c - 32];

    for (int row = 0; row < 14; row++)
{
    uint8_t bits = glyph[row];

    for (int col = 0; col < 8; col++)
    {
        if (bits & (0x80 >> col))
        {
            frame_buffer[(y + row) * DISPLAY_WIDTH
                       + (x + col)] = color;
        }
    }
}
}
uint16_t color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t c0, c1, c2;
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
    uint16_t r5 = (c0 >> 3) & 0x1F; // 5 bits
    uint16_t g6 = (c1 >> 2) & 0x3F; // 6 bits
    uint16_t b5 = (c2 >> 3) & 0x1F; // 5 bits

    return (r5 << 11) | (g6 << 5) | b5;
}
void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0)
        return;

    draw_line(x,         y,         x + w - 1, y,         color);
    draw_line(x,         y,         x,         y + h - 1, color);
    draw_line(x + w - 1, y,         x + w - 1, y + h - 1, color);
    draw_line(x,         y + h - 1, x + w - 1, y + h - 1, color);
}
void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0)
        return;

    for (int iy = 0; iy < h; iy++)
    {
        draw_line(x, y + iy,
                      x + w - 1, y + iy,
                      color);
    }
}
void draw_text(int x, int y, const char *str, uint16_t color) {
while (*str)
    {
        draw_char(
            x,
            y,
            *str,
            color
        );

        x += 9; // 8 pixels + 1 spacing
        str++;
    }
}

void push_frame(void) {
    if (frame_buffer == NULL) {
        ESP_LOGE(TAG, "frame_buffer is NULL");
        return;
    }

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
void draw_image( int dst_x, int dst_y, const uint8_t *img, int width, int height) {
    if (img == NULL) {
        return;
    }
    int index = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t pixel_rgb = img[index] << 8 | img[index + 1];

            
            draw_pixel(dst_x + x, dst_y + y, pixel_rgb);
            index += 2;
        }
    }
}

void push_buffer_to_frame(uint16_t *img_buf)
{
    ESP_LOGI(TAG,
             "frame_buffer=%p img_buf=%p",
             frame_buffer,
             img_buf);

    if (frame_buffer == NULL) {
        ESP_LOGE(TAG, "frame_buffer is NULL");
        return;
    }

    if (img_buf == NULL) {
        ESP_LOGE(TAG, "img_buf is NULL");
        return;
    }

    memcpy(frame_buffer,
           img_buf,
           FRAME_BUFFER_SIZE);
}