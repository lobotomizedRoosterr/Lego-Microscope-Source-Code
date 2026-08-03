#include "components/include/ui.h"
#include "components/include/render_engine.h"

button_t live_view_button, settings_button, gallery_button, debug_button;


enum screen {
    LIVE_VIEW,
    SETTINGS,
    GALLERY,
    DEBUG
};

static void on_button_select(void* ctx) {
    (void) ctx;

    switch((int) ctx) {
        case LIVE_VIEW:
            ui_navigate_push(&live_view_screen);
            break;
        case SETTINGS:
            ui_navigate_push(&settings_screen);
            break;
        case GALLERY:
            ui_navigate_push(&gallery_screen);
            break;
        case DEBUG:
            ui_navigate_push(&mode_debug_screen);
            break;
        default:
            break;
    }
}

static void directory_on_enter(void) {
    live_view_button = (button_t){
        .bounds = {
            .x = 35,
            .y = 10,
            .w = 105,
            .h = 105,
        },
        .hit_area = {
            .x = 35,
            .y = 10,
            .w = 105,
            .h = 105,
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
            .v_align=V_ALIGNMENT_MIDDLE
        },
        .label = "Live View",
        .on_press = on_button_select,
        .callback_ctx = (void *)(uintptr_t) LIVE_VIEW,
    };
    
    settings_button = (button_t){
        .bounds = {
            .x = 35+40+105,
            .y = 10,
            .w = 105,
            .h = 105,
        },
        .hit_area = {
            .x = 35+40+105,
            .y = 10,
            .w = 105,
            .h = 105,
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
            .v_align=V_ALIGNMENT_MIDDLE
        },
        .label = "Settings",
        .on_press = on_button_select,
        .callback_ctx = (void *)(uintptr_t) SETTINGS,
    };
    
    gallery_button = (button_t){
        .bounds = {
            .x = 35,
            .y = 10+10+105,
            .w = 105,
            .h = 105,
        },
        .hit_area = {
            .x = 35,
            .y = 10+10+105,
            .w = 105,
            .h = 105,
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
            .v_align=V_ALIGNMENT_MIDDLE
        },
        .label = "Gallery",
        .on_press = on_button_select,
        .callback_ctx = (void *)(uintptr_t) GALLERY,
    };
    
    debug_button = (button_t){
        .bounds = {
            .x = 35+40+105,
            .y = 10+10+105,
            .w = 105,
            .h = 105,
        },
        .hit_area = {
            .x = 35+40+105,
            .y = 10+10+105,
            .w = 105,
            .h = 105,
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
            .v_align=V_ALIGNMENT_MIDDLE
        },
        .label = "Debug",
        .on_press = on_button_select,
        .callback_ctx = (void *)(uintptr_t) DEBUG,
    };
}
 
static void directory_on_exit(void) {
    
}
 
static void directory_render(void) {
    fill_rect(0, 0, 320, 240, (rgb666_color_t){6,6,6});
    render_button(&live_view_button, false);
    render_button(&settings_button, false);
    render_button(&gallery_button, false);
    render_button(&debug_button, false);
}
 
static void directory_handle_touch(touch_event_t evt) {
    if (button_hit_test(&settings_button, evt) && !evt.pressed_last_tick) {
        if (settings_button.on_press != NULL) {
            settings_button.on_press(settings_button.callback_ctx);
        }
    }
    if (button_hit_test(&live_view_button, evt) && !evt.pressed_last_tick) {
        if (live_view_button.on_press != NULL) {
            live_view_button.on_press(live_view_button.callback_ctx);
        }
    }
    if (button_hit_test(&gallery_button, evt) && !evt.pressed_last_tick) {
        if (gallery_button.on_press != NULL) {
            gallery_button.on_press(gallery_button.callback_ctx);
        }
    }
    if (button_hit_test(&debug_button, evt) && !evt.pressed_last_tick) {
        if (debug_button.on_press != NULL) {
            debug_button.on_press(debug_button.callback_ctx);
        }
    }

}
 
static void directory_handle_tick(void) {

}
 
const screen_t directory_screen = {
    .handle_tick=&directory_handle_tick,
    .handle_touch=&directory_handle_touch,
    .name="Directory",
    .on_enter=&directory_on_enter,
    .on_exit=&directory_on_exit,
    .render=&directory_render
};