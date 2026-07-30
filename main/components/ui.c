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

/*
UI.c

Buttons
Every Button is stored as a struct.
Every button has a "mode" identifier. This dictates what to do when the button is selected.

*/

/* this is meant to hold data regarding a buttons situation.*/
typedef struct {
    bool was_selected_last_tick; /*If the button was selected last tick*/
} button_state_t;


button_t buttons[50];
int button_count = 0;

button_state_t button_states[50];

bool was_tap_down = false;

bool debug_mode = false;

void render_button(button_t b) {
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
void dpc_mode_button_pressed(button_t* self, void* data) {
    
    buttons[DF_BUTTON_ID].is_selected=false;
    buttons[BF_BUTTON_ID].is_selected=false;
    buttons[QDF_BUTTON_ID].is_selected=false;
    buttons[DPC_BUTTON_ID].is_selected=true;
    switch(current_mode) {
        case DPC_LR_MODE:
            set_mode(DPC_RL_MODE);
            self->raw_icon=dpc_rl_icon_raw;
            break;
        case DPC_RL_MODE:
            set_mode(DPC_TB_MODE);
            self->raw_icon=dpc_tb_icon_raw;
            break;
        case DPC_TB_MODE:
            set_mode(DPC_BT_MODE);
            self->raw_icon=dpc_bt_icon_raw;
            break;
        default:
            set_mode(DPC_LR_MODE);
            self->raw_icon=dpc_lr_icon_raw;
            break;
    }
}
void mode_button_pressed(button_t* self, void* data) {
    switch(self->id) {
        case DF_BUTTON_ID:
            set_mode(DF_MODE);
            buttons[DF_BUTTON_ID].is_selected=true;
            buttons[BF_BUTTON_ID].is_selected=false;
            buttons[QDF_BUTTON_ID].is_selected=false;
            buttons[DPC_BUTTON_ID].is_selected=false;
            break;
        
        case BF_BUTTON_ID:
            set_mode(BF_MODE);
            buttons[BF_BUTTON_ID].is_selected=true;
            buttons[DF_BUTTON_ID].is_selected=false;
            buttons[QDF_BUTTON_ID].is_selected=false;
            buttons[DPC_BUTTON_ID].is_selected=false;
            break;
        
        
        case QDF_BUTTON_ID:
            set_mode(QDF_MODE);
            buttons[QDF_BUTTON_ID].is_selected=true;
            buttons[DF_BUTTON_ID].is_selected=false;
            buttons[BF_BUTTON_ID].is_selected=false;
            buttons[DPC_BUTTON_ID].is_selected=false;
            break;
    };
}

esp_err_t init_ui(void) {

    // Here are the definitions for every UI Button
    //Note that the touch bounds are larger than needed. This is because many touch displays
    //are low quality. This should ensure that buttons can be pressed
    // see ui.h for 

    add_button((button_t) {
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
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0),
        .id=DF_BUTTON_ID,
        .on_select=&mode_button_pressed,
        .behavior=PUSH_BEHAVIOR
    });
    
    add_button((button_t) {
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
        .fg_color_selected=color_from_rgb(50, 0, 0),
        .id=BF_BUTTON_ID,
        .on_select=&mode_button_pressed,
        .behavior=PUSH_BEHAVIOR
    });
    
    add_button((button_t) {
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
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0),
        .id=QDF_BUTTON_ID,
        .on_select=&mode_button_pressed,
        .behavior=PUSH_BEHAVIOR
    });
    
    add_button((button_t) {
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
        .bg_color_selected=color_from_rgb(150, 25, 25),
        .fg_color_selected=color_from_rgb(50, 0, 0),
        .id=DPC_BUTTON_ID,
        .on_select=&dpc_mode_button_pressed,
        .behavior=PUSH_BEHAVIOR
    });
    return ESP_OK;
}

bool is_button_tapped(button_t *button, touch_data td) {
    bool ret = (button->t_x0 <= td.touch_x && button->t_x1 >= td.touch_x
                && button->t_y0 <= td.touch_y && button->t_y1 >= td.touch_y
                && td.pressed) ? true : false;
    return ret;
}
void update_ui(void) {
    touch_data td = get_touch();

    //this becomes true if even one button has been activated. It disallows buttons declared exclusive from activating.
    bool has_button_activated = false;
    for(int i = 0; i < button_count; i++) {
        button_t *b = &buttons[i];
        button_state_t *s = &button_states[i];

        // if button is not tapped, do not update; continue
        if(!is_button_tapped(b, td)) {
            s->was_selected_last_tick=false;
            continue;
        };

        // if button is push behavior, and was selected last tick, continue
        if(b->behavior == PUSH_BEHAVIOR && s->was_selected_last_tick) continue;

        //if button is exclusive and another button has been activated, do not update; continue
        if(b->is_exclusive == true && has_button_activated) continue;

        // if not continued, button is being interacted with properly
        s->was_selected_last_tick=true;
        has_button_activated=true;
        b->on_select(b, NULL);
    }

    //was_tap_down makes sure that the user has to release their finger before another button register can happen.
}

void add_button(button_t b) {
    buttons[b.id] = b;
    button_count++;
    button_states[b.id] = (button_state_t) {
        .was_selected_last_tick=false,
    };
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
