#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "stdbool.h"

typedef struct {
    int t_x0;
    int t_x1;
    int t_y0;
    int t_y1;
    int x;
    int y;
    int width;
    int height;
    uint16_t bg_color_normal;
    uint16_t bg_color_selected;
    uint16_t fg_color_normal;
    uint16_t fg_color_selected;
    char* label;
    int label_offset_x;
    int label_offset_y;
    const uint8_t* raw_icon;
    int icon_width;
    int icon_height;
    int icon_offset_x;
    int icon_offset_y;
    bool is_selected;
    bool is_visible;
    int mode;
} button;

esp_err_t init_ui(void);

extern volatile float cam_fps;
extern volatile float comp_fps;

void render_ui(void);
void add_button(button b);
void update_ui(void);
void close_gallery();