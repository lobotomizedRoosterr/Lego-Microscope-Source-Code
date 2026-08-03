#include "components/include/ui.h"
#include "components/include/mode_registry.h"
#include "components/include/render_engine.h"
#include "components/include/image_store.h"

#include "computation/include/computation.h"

#include "resources/images/bf_img.h"
#include "resources/images/df_img.h"
#include "resources/images/qdf_img.h"

#include "resources/images/dpc_lr_img.h"
#include "resources/images/dpc_rl_img.h"
#include "resources/images/dpc_tb_img.h"
#include "resources/images/dpc_bt_img.h"

#include "resources/images/exit_img.h"

#include "resources/images/flop_img.h"

 
static button_t mode_buttons[4];
 
static button_t capture_button;


static button_t exit_button;

const char** lab;


static void on_mode_press(void *ctx)
{
    
    uint8_t index = (uint8_t)(uintptr_t)ctx;

    if(index == DPC_GEN_MODE) {
        switch(current_mode) {
            case DPC_LR_MODE:
                set_mode(DPC_RL_MODE);
                break;
            case DPC_RL_MODE:
                set_mode(DPC_TB_MODE);
                break;
            case DPC_TB_MODE:
                set_mode(DPC_BT_MODE);
                break;
            default:
                set_mode(DPC_LR_MODE);
                break;
        }
        return;
    }

    set_mode(index);
}

static void on_capture_press(void *ctx) {
    (void) ctx;

    fill_rect_bounds(0,0,320,240,(rgb666_color_t){12,12,12});
    draw_text(106,113,"Saving Image", (rgb666_color_t){63,63,63}, 0);
    push_frame();

    uint32_t ind = 0;

    xSemaphoreTake(computation_frame_mutex, portMAX_DELAY);
    esp_err_t err = image_store_save((void*) computation_frame, 320, 240, IMAGE_FORMAT_U8_GRAY, current_mode, &ind);
    xSemaphoreGive(computation_frame_mutex);

    if(err != ESP_OK) {
        ESP_LOGI("Capture", "Failed to capture image");
        return;
    }
    ESP_LOGI("Capture", "Saved image at index %d", (int) ind);
}

static void on_exit_press(void *ctx) {
    ui_navigate_pop();
}

static void build_mode_buttons(void) {
 
    mode_buttons[BF_MODE] = (button_t){
        .bounds = {
            .x = 320-32,
            .y = 0,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 320-48,
            .y = 0,
            .w = 48,
            .h = 48,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .icon = {
            .raw = &brightfield_icon_raw,
            .w=32,
            .h=32
        },
        .label = "BF",
        .on_press = on_mode_press,
        .callback_ctx = (void *)(uintptr_t)BF_MODE,
    };
    mode_buttons[DF_MODE] = (button_t){
        .bounds = {
            .x = 320-32,
            .y = 48,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 320-48,
            .y = 48,
            .w = 48,
            .h = 48,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .icon = {
            .raw = &darkfield_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .label = "DF",
        .on_press = on_mode_press,
        .callback_ctx = (void *)(uintptr_t)DF_MODE,
    };
    mode_buttons[QDF_MODE] = (button_t){
        .bounds = {
            .x = 320-32,
            .y = 48*2,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 320-48,
            .y = 48*2,
            .w = 48,
            .h = 48,
        },
        .icon = {
            .raw = &qdf_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .label = "QDF",
        .on_press = on_mode_press,
        .callback_ctx = (void *)(uintptr_t)QDF_MODE,
    };
    mode_buttons[DPC_GEN_MODE] = (button_t){
        .bounds = {
            .x = 320-32,
            .y = 48*3,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 320-48,
            .y = 48*3,
            .w = 48,
            .h = 48,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .icon = {
            .raw = &dpc_lr_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .label = "DPC",
        .on_press = on_mode_press,
        .callback_ctx = (void *)(uintptr_t)DPC_GEN_MODE,
    };
    


    
}
static void build_capture_button(void) {
    capture_button = (button_t){
        .bounds = {
            .x = 320-32,
            .y = 48*4,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 320-48,
            .y = 48*4,
            .w = 48,
            .h = 48,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .icon = {
            .raw = &flop_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .label = "CAP",
        .on_press = on_capture_press,
        .callback_ctx = NULL,
    };
}
static void build_exit_button(void) {
    exit_button = (button_t){
        .bounds = {
            .x = 0,
            .y = 48*4,
            .w = 32,
            .h = 48,
        },
        .hit_area = {
            .x = 0,
            .y = 48*4,
            .w = 48,
            .h = 48,
        },
        .colors = {
            .normal   = {
                .fg = { .r = 63, .g = 63, .b = 63 },
                .bg = { .r = 12, .g = 12, .b = 12 },
            },
            .selected = {
                .fg = { .r = 0,  .g = 0,  .b = 0  },
                .bg = { .r = 0,  .g = 45, .b = 63 },
            },
        },
        .icon = {
            .raw = &exit_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .border = {
            .color = { .r = 30, .g = 30, .b = 30 },
            .width = 1,
        },
        .text_style = {
            .font  = NULL,   /* default font */
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .label = "EXT",
        .on_press = on_exit_press,
        .callback_ctx = NULL,
    };
}
static void live_view_on_enter(void) {
    build_mode_buttons();
    build_capture_button();
    build_exit_button();
}
 
static void live_view_on_exit(void) {
    
}
 
static void live_view_render(void) {
    xSemaphoreTake(computation_frame_mutex, portMAX_DELAY);
    draw_greyscale_image(computation_frame, 0, 0, 320, 240);
    xSemaphoreGive(computation_frame_mutex);


    fill_rect_bounds(0, 0, 320,16, (rgb666_color_t){12, 12,12});
    draw_rect_bounds(0,0,320,16,(rgb666_color_t){30,30,30});
    draw_text(1,1,get_mode_from_id(current_mode).label, (rgb666_color_t){63,63,63},0);


    for (uint8_t i = 0; i < 4; i++) {
        bool selected = (i==current_mode);
        if(((current_mode == DPC_LR_MODE) || (current_mode == DPC_RL_MODE) || (current_mode == DPC_TB_MODE) || (current_mode == DPC_BT_MODE)) && i == DPC_GEN_MODE) {
            selected=true;
        }
        render_button(&mode_buttons[i], selected);
    }
    render_button(&capture_button, false);
    render_button(&exit_button, false);
}
 
static void live_view_handle_touch(touch_event_t evt) {
    for (uint8_t i = 0; i < 4; i++) {
        if (button_hit_test(&mode_buttons[i], evt) && !evt.pressed_last_tick) {
            if (mode_buttons[i].on_press != NULL) {
                mode_buttons[i].on_press(mode_buttons[i].callback_ctx);
            }
            break;
        }
    }
    
        if (button_hit_test(&capture_button, evt) && !evt.pressed_last_tick) {
            if (capture_button.on_press != NULL) {
                capture_button.on_press(capture_button.callback_ctx);
            }
        }
        if (button_hit_test(&exit_button, evt) && !evt.pressed_last_tick) {
            if (exit_button.on_press != NULL) {
                exit_button.on_press(exit_button.callback_ctx);
            }
        }

}
 
static void live_view_handle_tick(void) {

}
 
const screen_t live_view_screen = {
    .handle_tick=&live_view_handle_tick,
    .handle_touch=&live_view_handle_touch,
    .name="Live View",
    .on_enter=&live_view_on_enter,
    .on_exit=&live_view_on_exit,
    .render=&live_view_render
};