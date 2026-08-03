#include "components/include/ui.h"
#include "components/include/render_engine.h"

static button_t exit_button;

static void on_exit_press(void* ctx) {
    (void) ctx;
    ui_navigate_pop();
}

static void debug_on_enter(void) {
    
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
 
static void debug_on_exit(void) {
    
}
 
static void debug_render(void) {
    fill_rect(0,0,320,240,(rgb666_color_t){0,0,0});
    draw_text(320/2-153/2, 240/2-14/2,"Under Construction", (rgb666_color_t){63,0,0}, 0);
    draw_text(320/2-153/2+1, 240/2-14/2+1,"Under Construction", (rgb666_color_t){31,0,0}, 0);
    render_button(&exit_button, false);
}
 
static void debug_handle_touch(touch_event_t evt) {
    if (button_hit_test(&exit_button, evt) && !evt.pressed_last_tick) {
        if (exit_button.on_press != NULL) {
            exit_button.on_press(exit_button.callback_ctx);
        }
    }
}
 
static void debug_handle_tick(void) {

}
 
const screen_t mode_debug_screen = {
    .handle_tick=&debug_handle_tick,
    .handle_touch=&debug_handle_touch,
    .name="Debug",
    .on_enter=&debug_on_enter,
    .on_exit=&debug_on_exit,
    .render=&debug_render
};