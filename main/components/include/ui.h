#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "stdbool.h"


typedef struct {
    int t_x0; /*first touch boundary x value*/
    int t_x1; /*second touch boundary x value*/
    int t_y0; /*first touch boundary y value*/
    int t_y1; /*second touch boundary y value*/
    int x; /*x value for button location*/
    int y;/*y value for button location*/
    int width; /*width of button*/
    int height; /*height of button*/
    uint16_t bg_color_normal; /*background color of button when not selected*/
    uint16_t bg_color_selected; /*background color of button when is selected*/
    uint16_t fg_color_normal; /*foreground color of button when not selected*/
    uint16_t fg_color_selected; /*foreground color of button when is selected*/
    char* label; /*button text label*/
    int label_offset_x; /*label position relative to button position - x value*/
    int label_offset_y; /*label position relative to button position - y value*/
    const uint8_t* raw_icon; /*raw data for the icon of the button*/
    int icon_width; /*width of icon*/
    int icon_height; /* height of icon*/
    int icon_offset_x; /*icon position relative to button position - x value*/
    int icon_offset_y; /*icon position relative to button position - y value*/
    bool is_selected; /*if button is selected*/
    bool is_visible; /*if button should be rendered*/
    int mode; /*identifier used to determine actions when button is selected*/
} button;

/* Creates UI objects, general initialization*/
esp_err_t init_ui(void);

/* render UI objects*/
void render_ui(void);
/* add button to storage, increase number of registered buttons*/
void add_button(button b);
/* update ui, (touch controls, etc.)*/
void update_ui(void);
