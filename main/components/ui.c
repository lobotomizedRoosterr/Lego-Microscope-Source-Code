#include "components/include/ui.h"
#include "components/include/render_engine.h"
#include "components/include/display.h"
#include "esp_err.h"
#include "main/app_state.h"
#include "components/include/mode_registry.h"
#include "components/include/touch.h"
#include "components/include/acquisition_sequencer.h"
#include "esp_log.h"
#include "computation/include/computation.h"
#include "images/bug_img.h"
#include "images/df_img.h"
#include "images/bf_img.h"
#include "images/qdf_img.h"
#include "images/dpc_img.h"
#include "images/dpc_lr_img.h"
#include "images/dpc_rl_img.h"
#include "images/dpc_tb_img.h"
#include "images/dpc_bt_img.h"
#include "images/bgc_img.h"
#include "images/file_img.h"

#define BUG_MODE 99
#define DPC_GEN_MODE 98

/*
UI.c

Buttons
Every Button is stored as a struct.
Every button has a "mode" identifier. This dictates what to do when the button is selected.

*/


// max buttons = 50
button buttons[50];
int button_count = 0;

bool was_tap_down = false;

volatile float cam_fps = 0.0f;
volatile float comp_fps = 0.0f;

bool debug_mode = false;

void render_button(button b) {
    uint16_t bg_color;
    uint16_t fg_color;
    if(b.is_selected) {
        bg_color=b.bg_color_selected;
        fg_color=b.fg_color_selected;
    } else {
        bg_color=b.bg_color_normal;
        fg_color=b.fg_color_normal;
    }
    fill_rect(b.x, b.y, b.width, b.height, bg_color);
    if(b.raw_icon != NULL) {
        draw_image(b.x+b.icon_offset_x, b.y+b.icon_offset_y, b.raw_icon, b.icon_width, b.icon_height);
    }
    draw_rect(b.x, b.y, b.width, b.height, fg_color);

    draw_text(b.x+b.label_offset_x, b.y+b.label_offset_y, b.label, fg_color);
}

esp_err_t init_ui(void) {

    // Here are the definitions for every UI Button
    //Note that the touch bounds are larger than needed. This is because many touch displays
    //are low quality. This should ensure that buttons can be pressed
    // see ui.h for 

    add_button((button) {
        .bg_color_normal=color_from_rgb(0, 0, 0),
        .fg_color_normal=color_from_rgb(255, 255, 255),
        .icon_offset_x=0,
        .icon_offset_y=0,
        .height=48,
        .width=32,
        .is_selected=true,
        .label_offset_x=8,
        .label_offset_y=33,
        .label="DF",
        .x=288,
        .t_x0=258,
        .t_x1=320,
        .t_y0=0,
        .t_y1=48,
        .y=0,
        .raw_icon=darkfield_icon_raw,
        .icon_width=32,
        .icon_height=32,
        .mode=DF_MODE,
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0)
    });
    
    add_button((button) {
        .bg_color_normal=color_from_rgb(0, 0, 0),
        .fg_color_normal=color_from_rgb(255, 255, 255),
        .icon_offset_x=0,
        .icon_offset_y=0,
        .height=48,
        .width=32,
        .is_selected=false,
        .label_offset_x=8,
        .label_offset_y=33,
        .label="BF",
        .x=288,
        .t_x0=258,
        .t_x1=320,
        .t_y0=48,
        .t_y1=96,
        .y=48,
        .raw_icon=brightfield_icon_raw,
        .icon_width=32,
        .icon_height=32,
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .mode=BF_MODE,
        .fg_color_selected=color_from_rgb(50, 0, 0)
    });
    
    add_button((button) {
        .bg_color_normal=color_from_rgb(0, 0, 0),
        .fg_color_normal=color_from_rgb(255, 255, 255),
        .icon_offset_x=0,
        .icon_offset_y=0,
        .height=48,
        .width=32,
        .is_selected=false,
        .label_offset_x=3,
        .label_offset_y=33,
        .label="QDF",
        .x=288,
        .y=48*2,
        .raw_icon=qdf_icon_raw,
        .icon_width=32,
        .icon_height=32,
        .t_x0=258,
        .t_x1=320,
        .t_y0=96,
        .t_y1=144,
        .mode=QDF_MODE,
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0)
    });
    
    add_button((button) {
        .bg_color_normal=color_from_rgb(0, 0, 0),
        .fg_color_normal=color_from_rgb(255, 255, 255),
        .icon_offset_x=0,
        .icon_offset_y=0,
        .height=48,
        .width=32,
        .is_selected=false,
        .label_offset_x=3,
        .label_offset_y=33,
        .label="DPC",
        .x=288,
        .t_x0=258,
        .t_x1=320,
        .y=48*3,
        .t_y0=48*3,
        .t_y1=48*4,
        .raw_icon=dpc_icon_raw,
        .icon_width=32,
        .icon_height=32,
        .mode=DPC_GEN_MODE,
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0)
    });
    return ESP_OK;
}
void update_ui(void) {
    touch_data td = get_touch();

    /*
    TODO Change this
    This needs to be reformatted and simplified.
    Each button needs to contain a pointer to a function that passes values like *button self that runs when selected.
    */

    //was_tap_down makes sure that the user has to release their finger before another button register can happen.
    if(!was_tap_down && td.pressed) {
        //Cycle through every button to update
        for(int i = 0; i < button_count; i++) {
            //Is touch point within bounds?
            if(td.touch_x > buttons[i].t_x0 && td.touch_x < buttons[i].t_x1 && td.touch_y > buttons[i].t_y0 && td.touch_y<buttons[i].t_y1) {
                //This is a remnant of a previous button, Debug. This is here to make sure that there are no attempts to set mode to "BUG_MODE"
                if(buttons[i].mode == BUG_MODE) {
                    continue;
                }
                //The DPC button works where it gets mode, if it is a mode other than DPC it sets it to DPC LR
                //Then, if it was DPC, it has it cycle through the different modes.
                if(buttons[i].mode == DPC_GEN_MODE) {
                    buttons[i].is_selected=true;
                    switch(current_mode) {
                        case DPC_LR_MODE:
                        set_mode(DPC_RL_MODE);
                        buttons[i].raw_icon=dpc_rl_icon_raw;
                        break;
                        
                        case DPC_RL_MODE:
                        set_mode(DPC_TB_MODE);
                        buttons[i].raw_icon=dpc_tb_icon_raw;
                        break;
                        
                        case DPC_TB_MODE:
                        set_mode(DPC_BT_MODE);
                        buttons[i].raw_icon=dpc_bt_icon_raw;
                        break;
                        
                        case DPC_BT_MODE:
                        set_mode(DPC_LR_MODE);
                        buttons[i].raw_icon=dpc_lr_icon_raw;
                        break;

                        default:
                        set_mode(DPC_LR_MODE);
                        buttons[i].raw_icon=dpc_rl_icon_raw;
                        break;
                    }
                    //unselect every other button (except for i, the one that was tapped)
                    for(int j = 0; j < button_count; j++) {
                        if(j != i) {
                            buttons[j].is_selected=false;
                        }
                    }
                    continue;
                }
                    //unselect every other button (except for i, the one that was tapped)
                for(int j = 0; j < button_count; j++) {
                    if(j != i) {
                        buttons[j].is_selected=false;
                    }
                }
                //select button
                buttons[i].is_selected=true;
                //set mode
                set_mode(buttons[i].mode);

            } 
        }
        //make sure that the same button press isn't detected next tick.
        was_tap_down=true;
    }
    // if no touch is detected, set was_tap_down accordingly, for the next tick
    if(was_tap_down && !td.pressed) {
        was_tap_down=false;
    }
}

void add_button(button b) {
    buttons[button_count] = b;
    button_count++;
}

void render_ui(void) {
    //render every button
    for(int i = 0; i < button_count; i++) {
        render_button(buttons[i]);
    }
    //render the top bar that displays mode
    fill_rect(0, 0, 288, 16, color_from_rgb(0, 0, 0));
    draw_text(1, 1, get_mode_from_id(current_mode).label, color_from_rgb(0, 255, 0));

}
