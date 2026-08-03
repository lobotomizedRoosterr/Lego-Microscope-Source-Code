#include "components/include/ui.h"
#include "components/include/render_engine.h"
#include "components/include/mode_registry.h"
#include "resources/images/exit_img.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static button_t exit_button;

/* ---- mode group definitions ---- */
/* DPC LR/RL/TB/BT are treated as one configurable entity — editing
 * "DPC" writes the same camera config to all four underlying mode ids
 * so they stay in sync. */

typedef enum {
    UI_MODE_BF = 0,
    UI_MODE_DF,
    UI_MODE_QDF,
    UI_MODE_DPC,
    UI_MODE_GROUP_COUNT
} ui_mode_group_t;

static const char* mode_group_labels[UI_MODE_GROUP_COUNT] = {
    "BF", "DF", "QDF", "DPC"
};

/* underlying mode_registry ids each group writes to */
static const int mode_group_ids[UI_MODE_GROUP_COUNT][4] = {
    { BF_MODE,    -1,          -1,          -1          },
    { DF_MODE,    -1,          -1,          -1          },
    { QDF_MODE,   -1,          -1,          -1          },
    { DPC_LR_MODE, DPC_RL_MODE, DPC_TB_MODE, DPC_BT_MODE },
};
static const int mode_group_id_count[UI_MODE_GROUP_COUNT] = { 1, 1, 1, 4 };

/* ---- editable field definitions ---- */
/* generic access into microscopy_config via offsetof, so one set of
 * dec/inc callbacks handles all 5 fields */

typedef struct {
    const char* label;
    int min, max, step;
    size_t offset;
} field_def_t;

static const field_def_t field_defs[5] = {
    { "Exposure",     0,   1200, 25, offsetof(microscopy_config, exposure_val)      },
    { "Gain",         0,   30,   1,  offsetof(microscopy_config, gain_val)          },
    { "Exp. Time",    0,   200,  1,  offsetof(microscopy_config, exposure_time)     },
    { "Sharpness",   -2,   2,    1,  offsetof(microscopy_config, sharpness_val)     },
    { "LED Bright.",  0,   15,   1,  offsetof(microscopy_config, led_brightness_val)},
};
#define NUM_FIELDS 5

static int* field_ptr(microscopy_config* cfg, int idx) {
    return (int*)((uint8_t*)cfg + field_defs[idx].offset);
}

static int clampi(int v, int mn, int mx) {
    return v < mn ? mn : (v > mx ? mx : v);
}

/* ---- state ---- */

typedef enum {
    SETTINGS_VIEW_LIST,
    SETTINGS_VIEW_EDIT
} settings_view_t;

static settings_view_t current_view;
static int selected_group;
static microscopy_config edit_cfg;

/* ---- buttons ---- */

static button_t mode_buttons[UI_MODE_GROUP_COUNT];
static button_t back_button;
static button_t save_button;
static button_t field_dec_buttons[NUM_FIELDS];
static button_t field_inc_buttons[NUM_FIELDS];

static button_t make_button(int x, int y, int w, int h, const char* label,
                             void (*on_press)(void*), void* ctx) {
    return (button_t){
        .bounds = { .x = x, .y = y, .w = w, .h = h },
        .hit_area = { .x = x, .y = y, .w = w, .h = h },
        .colors = {
            .normal   = { .fg = { 63, 63, 63 }, .bg = { 12, 12, 12 } },
            .selected = { .fg = { 0, 0, 0 },    .bg = { 0, 45, 63 } },
        },
        .border = { .color = { 30, 30, 30 }, .width = 1 },
        .text_style = {
            .font = NULL,
            .color = { 63, 63, 63 },
            .h_align = H_ALIGNMENT_CENTER,
            .v_align = V_ALIGNMENT_MIDDLE,
        },
        .icon = {0},
        .label = label,
        .on_press = on_press,
        .callback_ctx = ctx,
    };
}

/* ---- callbacks ---- */

static void on_exit_press(void* ctx) {
    (void) ctx;
    ui_navigate_pop();
}

static void load_edit_cfg_for_group(int group) {
    int first_id = mode_group_ids[group][0];
    if (mode_config_get(first_id, &edit_cfg) != ESP_OK) {
        // fall back to whatever's currently live in the mode struct
        microscopy_mode m = get_mode_from_id(first_id);
        edit_cfg.exposure_val = m.exposure_val;
        edit_cfg.gain_val = m.gain_val;
        edit_cfg.exposure_time = m.exposure_time;
        edit_cfg.sharpness_val = m.sharpness_val;
        edit_cfg.led_brightness_val = m.led_brightness_val;
    }
}

static void on_mode_select(void* ctx) {
    int group = (int)(intptr_t) ctx;
    selected_group = group;
    load_edit_cfg_for_group(group);
    current_view = SETTINGS_VIEW_EDIT;
}

static void on_field_dec(void* ctx) {
    int idx = (int)(intptr_t) ctx;
    int* v = field_ptr(&edit_cfg, idx);
    *v = clampi(*v - field_defs[idx].step, field_defs[idx].min, field_defs[idx].max);
}

static void on_field_inc(void* ctx) {
    int idx = (int)(intptr_t) ctx;
    int* v = field_ptr(&edit_cfg, idx);
    *v = clampi(*v + field_defs[idx].step, field_defs[idx].min, field_defs[idx].max);
}

static void on_save(void* ctx) {
    (void) ctx;
    int count = mode_group_id_count[selected_group];
    for (int i = 0; i < count; i++) {
        int mode_id = mode_group_ids[selected_group][i];
        esp_err_t err = mode_config_set(mode_id, &edit_cfg);
        if (err != ESP_OK) {
            ESP_LOGE("SETTINGS", "Failed saving config for mode %d: %s",
                     mode_id, esp_err_to_name(err));
        }
    }
    current_view = SETTINGS_VIEW_LIST;
}

static void on_back(void* ctx) {
    (void) ctx;
    // discard edits, return to the mode list
    current_view = SETTINGS_VIEW_LIST;
}

/* ---- screen lifecycle ---- */

static void settings_on_enter(void) {
    current_view = SETTINGS_VIEW_LIST;

    for (int i = 0; i < UI_MODE_GROUP_COUNT; i++) {
        mode_buttons[i] = make_button(
            100, 30 + i * 45, 180, 38,
            mode_group_labels[i], on_mode_select, (void*)(intptr_t) i);
    }

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
            .font  = NULL,
            .color = { .r = 63, .g = 63, .b = 63 },
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_BOTTOM
        },
        .icon = {
            .raw = &exit_icon_raw,
            .w=32,
            .h=32,
            .h_align=H_ALIGNMENT_CENTER,
            .v_align=V_ALIGNMENT_TOP,
        },
        .label = "EXT",
        .on_press = on_exit_press,
        .callback_ctx = NULL,
    };

    back_button = make_button(0, 190, 60, 40, "BACK", on_back, NULL);
    save_button = make_button(250, 190, 70, 40, "SAVE", on_save, NULL);

    for (int i = 0; i < NUM_FIELDS; i++) {
        int y = 30 + i * 30;
        field_dec_buttons[i] = make_button(140, y, 30, 24, "-", on_field_dec, (void*)(intptr_t) i);
        field_inc_buttons[i] = make_button(230, y, 30, 24, "+", on_field_inc, (void*)(intptr_t) i);
    }
}

static void settings_on_exit(void) {

}

static void render_list_view(void) {
    fill_rect(0,0,320,240,(rgb666_color_t){0,0,0});
    draw_text(100, 8, "Select mode to configure", (rgb666_color_t){63,63,63}, 0);
    for (int i = 0; i < UI_MODE_GROUP_COUNT; i++) {
        render_button(&mode_buttons[i], false);
    }
    render_button(&exit_button, false);
}

static void render_edit_view(void) {
    fill_rect(0,0,320,240,(rgb666_color_t){0,0,0});

    char title[32];
    snprintf(title, sizeof(title), "%s Config", mode_group_labels[selected_group]);
    draw_text(10, 8, title, (rgb666_color_t){63,63,63}, 0);

    for (int i = 0; i < NUM_FIELDS; i++) {
        int y = 30 + i * 30;
        draw_text(10, y + 6, field_defs[i].label, (rgb666_color_t){63,63,63}, 0);

        char valbuf[8];
        snprintf(valbuf, sizeof(valbuf), "%d", *field_ptr(&edit_cfg, i));
        draw_text(180, y + 6, valbuf, (rgb666_color_t){63,63,63}, 0);

        render_button(&field_dec_buttons[i], false);
        render_button(&field_inc_buttons[i], false);
    }

    render_button(&back_button, false);
    render_button(&save_button, false);
}

static void settings_render(void) {
    if (current_view == SETTINGS_VIEW_LIST) {
        render_list_view();
    } else {
        render_edit_view();
    }
}

static void handle_touch_list(touch_event_t evt) {
    for (int i = 0; i < UI_MODE_GROUP_COUNT; i++) {
        if (button_hit_test(&mode_buttons[i], evt) && !evt.pressed_last_tick) {
            if (mode_buttons[i].on_press != NULL) {
                mode_buttons[i].on_press(mode_buttons[i].callback_ctx);
            }
            return;
        }
    }
    if (button_hit_test(&exit_button, evt) && !evt.pressed_last_tick) {
        if (exit_button.on_press != NULL) {
            exit_button.on_press(exit_button.callback_ctx);
        }
    }
}

static void handle_touch_edit(touch_event_t evt) {
    if (evt.pressed_last_tick) {
        return;
    }
    for (int i = 0; i < NUM_FIELDS; i++) {
        if (button_hit_test(&field_dec_buttons[i], evt)) {
            field_dec_buttons[i].on_press(field_dec_buttons[i].callback_ctx);
            return;
        }
        if (button_hit_test(&field_inc_buttons[i], evt)) {
            field_inc_buttons[i].on_press(field_inc_buttons[i].callback_ctx);
            return;
        }
    }
    if (button_hit_test(&back_button, evt)) {
        back_button.on_press(back_button.callback_ctx);
        return;
    }
    if (button_hit_test(&save_button, evt)) {
        save_button.on_press(save_button.callback_ctx);
        return;
    }
}

static void settings_handle_touch(touch_event_t evt) {
    if (current_view == SETTINGS_VIEW_LIST) {
        handle_touch_list(evt);
    } else {
        handle_touch_edit(evt);
    }
}

static void settings_handle_tick(void) {

}

const screen_t settings_screen = {
    .handle_tick=&settings_handle_tick,
    .handle_touch=&settings_handle_touch,
    .name="Settings",
    .on_enter=&settings_on_enter,
    .on_exit=&settings_on_exit,
    .render=&settings_render
};