#include "components/include/ui.h"
#include "components/include/render_engine.h"
#include "components/include/image_store.h"
#include "resources/images/exit_img.h"
#include "components/include/mode_registry.h"
#include <stdio.h>
#include <stdlib.h>

#define GRID_COLS 4
#define GRID_ROWS 2
#define CELLS_PER_PAGE (GRID_COLS * GRID_ROWS)
#define THUMB_W 64
#define THUMB_H 64
#define CELL_W 74
#define CELL_H 88
#define GRID_ORIGIN_X 8
#define GRID_ORIGIN_Y 16

/* ---- shared state between gallery + detail screens ---- */

static grid_cell_t cells[CELLS_PER_PAGE];
static int cell_count_this_page;
static uint32_t current_page;
static uint32_t total_images;

static int selected_cell = -1;
static uint32_t selected_image_index;

static uint8_t thumb_scratch[THUMB_W * THUMB_H]; /* reused per-cell during render */


static uint8_t thumb_cache[CELLS_PER_PAGE][THUMB_W * THUMB_H];
static bool thumb_cache_valid[CELLS_PER_PAGE];

/* ---- buttons ---- */

static button_t back_button;
static button_t prev_page_button;
static button_t next_page_button;
static button_t detail_back_button;
static button_t detail_prev_button;
static button_t detail_next_button;
static button_t detail_delete_button;

static button_t make_button(int x, int y, int w, int h, const char* label,
                             button_callback_t on_press, void* ctx) {
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
/* ---- gallery (grid) screen ---- */

static void layout_page(void) {
    uint32_t start_index = current_page * CELLS_PER_PAGE;
    cell_count_this_page = 0;

    for (uint32_t i = start_index; i < IMAGE_STORE_MAX_IMAGES && cell_count_this_page < CELLS_PER_PAGE; i++) {
        if (!image_store_exists(i)) {
            continue;
        }
        int slot = cell_count_this_page;
        int col = slot % GRID_COLS;
        int row = slot / GRID_COLS;

        cells[slot].bounds.x = GRID_ORIGIN_X + col * CELL_W;
        cells[slot].bounds.y = GRID_ORIGIN_Y + row * CELL_H;
        cells[slot].bounds.w = THUMB_W;
        cells[slot].bounds.h = THUMB_H;
        cells[slot].image_index = i;

        // decode once here, not in render()
        thumb_cache_valid[slot] =
            (image_store_load_thumbnail(i, thumb_cache[slot], THUMB_W, THUMB_H) == ESP_OK);

        cell_count_this_page++;
    }
}

static void on_prev_page(void* ctx) {
    (void) ctx;
    if (current_page > 0) {
        current_page--;
        layout_page();
    }
}

static void on_next_page(void* ctx) {
    (void) ctx;
    uint32_t max_page = (total_images == 0) ? 0 : (total_images - 1) / CELLS_PER_PAGE;
    if (current_page < max_page) {
        current_page++;
        layout_page();
    }
}

static void on_gallery_exit(void* ctx) {
    (void) ctx;
    ui_navigate_pop();
}

static void gallery_on_enter(void) {
    current_page = 0;
    total_images = image_store_count();
    layout_page();

    back_button = (button_t){
        .bounds   = { .x = 0, .y = 192, .w = 48, .h = 48 },
        .hit_area = { .x = 0, .y = 192, .w = 48, .h = 48 },
        .colors = {
            .normal   = { .fg = { 63, 63, 63 }, .bg = { 12, 12, 12 } },
            .selected = { .fg = { 0, 0, 0 },    .bg = { 0, 45, 63 } },
        },
        .border = { .color = { 30, 30, 30 }, .width = 1 },
        .text_style = {
            .font = NULL,
            .color = { 63, 63, 63 },
            .h_align = H_ALIGNMENT_CENTER,
            .v_align = V_ALIGNMENT_BOTTOM
        },
        .icon = {
            .raw = &exit_icon_raw,
            .w = 32, .h = 32,
            .h_align = H_ALIGNMENT_CENTER,
            .v_align = V_ALIGNMENT_TOP,
        },
        .label = "EXT",
        .on_press = on_gallery_exit,
        .callback_ctx = NULL,
    };

    prev_page_button = make_button(240, 192, 36, 36, "<", on_prev_page, NULL);
    next_page_button = make_button(280, 192, 36, 36, ">", on_next_page, NULL);
}

static void gallery_on_exit(void) {

}
static void gallery_render(void) {
    fill_rect(0, 0, 320, 240, (rgb666_color_t){0, 0, 0});

    if (total_images == 0) {
        draw_text(320/2 - 60, 240/2 - 7, "No images saved", (rgb666_color_t){63, 63, 63}, 0);
    } else {
        for (int i = 0; i < cell_count_this_page; i++) {
            if (thumb_cache_valid[i]) {
                draw_greyscale_image(&thumb_cache[i], cells[i].bounds.x, cells[i].bounds.y, THUMB_W, THUMB_H);
            }
            render_grid_cell(&cells[i], i == selected_cell);
            char idx_label[8];
            snprintf(idx_label, sizeof(idx_label), "#%lu", (unsigned long) cells[i].image_index);
            draw_text(cells[i].bounds.x, cells[i].bounds.y + THUMB_H + 2, idx_label,
                       (rgb666_color_t){40, 40, 40}, 0);
        }

        uint32_t max_page = (total_images - 1) / CELLS_PER_PAGE;
        char page_label[16];
        snprintf(page_label, sizeof(page_label), "%lu / %lu",
                 (unsigned long) current_page + 1, (unsigned long) max_page + 1);
        draw_text(150, 200, page_label, (rgb666_color_t){63, 63, 63}, 0);
    }

    render_button(&prev_page_button, false);
    render_button(&next_page_button, false);
    render_button(&back_button, false);
}
static void gallery_handle_touch(touch_event_t evt) {
    if (evt.pressed_last_tick) {
        return;
    }

    for (int i = 0; i < cell_count_this_page; i++) {
        if (hit_test_rect(cells[i].bounds, evt.x, evt.y)) {
            selected_image_index = cells[i].image_index;
            ui_navigate_push(&image_detail_screen);
            return;
        }
    }

    if (button_hit_test(&prev_page_button, evt)) {
        prev_page_button.on_press(prev_page_button.callback_ctx);
        return;
    }
    if (button_hit_test(&next_page_button, evt)) {
        next_page_button.on_press(next_page_button.callback_ctx);
        return;
    }
    if (button_hit_test(&back_button, evt)) {
        back_button.on_press(back_button.callback_ctx);
        return;
    }
}

static void gallery_handle_tick(void) {

}

const screen_t gallery_screen = {
    .handle_tick   = &gallery_handle_tick,
    .handle_touch  = &gallery_handle_touch,
    .name          = "Gallery",
    .on_enter      = &gallery_on_enter,
    .on_exit       = &gallery_on_exit,
    .render        = &gallery_render,
};

/* ---- detail screen (single full image, prev/next/delete) ---- */

#define DETAIL_IMG_X 32
#define DETAIL_IMG_Y 16
#define DETAIL_IMG_W 240
#define DETAIL_IMG_H 180

static uint8_t detail_scratch[DETAIL_IMG_W * DETAIL_IMG_H];

static void step_detail_image(int delta) {
    int32_t idx = (int32_t) selected_image_index;
    for (int32_t i = 0; i < IMAGE_STORE_MAX_IMAGES; i++) {
        idx += delta;
        if (idx < 0) idx = IMAGE_STORE_MAX_IMAGES - 1;
        if (idx >= IMAGE_STORE_MAX_IMAGES) idx = 0;
        if (image_store_exists((uint32_t) idx)) {
            selected_image_index = (uint32_t) idx;
            return;
        }
    }
    /* no other images exist; stay put */
}

static void on_detail_prev(void* ctx) {
    (void) ctx;
    step_detail_image(-1);
}

static void on_detail_next(void* ctx) {
    (void) ctx;
    step_detail_image(1);
}

static void on_detail_back(void* ctx) {
    (void) ctx;
    ui_navigate_pop();
}

static void on_detail_delete(void* ctx) {
    (void) ctx;
    uint32_t to_delete = selected_image_index;
    step_detail_image(1); /* move off the image before deleting it */
    image_store_delete(to_delete);
    total_images = image_store_count();

    if (total_images == 0) {
        ui_navigate_pop();
    }
}

static void detail_on_enter(void) {
    detail_back_button   = make_button(0,   200, 60, 36, "BACK", on_detail_back, NULL);
    detail_prev_button   = make_button(70,  200, 50, 36, "<",    on_detail_prev, NULL);
    detail_next_button   = make_button(130, 200, 50, 36, ">",    on_detail_next, NULL);
    detail_delete_button = make_button(250, 200, 65, 36, "DEL",  on_detail_delete, NULL);
}

static void detail_on_exit(void) {

}
static uint32_t detail_cached_index = UINT32_MAX;
static bool detail_cache_valid = false;
static image_header_t detail_cached_header;

static void detail_reload_if_needed(void) {
    if (detail_cache_valid && detail_cached_index == selected_image_index) {
        return; // already have this one decoded
    }
    detail_cache_valid =
        (image_store_load_header(selected_image_index, &detail_cached_header) == ESP_OK) &&
        (image_store_load_thumbnail(selected_image_index, detail_scratch, DETAIL_IMG_W, DETAIL_IMG_H) == ESP_OK);
    detail_cached_index = selected_image_index;
}

static void detail_render(void) {
    fill_rect(0, 0, 320, 240, (rgb666_color_t){0, 0, 0});

    detail_reload_if_needed();

    if (detail_cache_valid) {
        draw_greyscale_image(&detail_scratch, DETAIL_IMG_X, DETAIL_IMG_Y, DETAIL_IMG_W, DETAIL_IMG_H);
        char info[48];
        snprintf(info, sizeof(info), "#%lu  mode %u  %ux%u",
                 (unsigned long) selected_image_index, detail_cached_header.mode_id,
                 detail_cached_header.width, detail_cached_header.height);
        draw_text(10, 4, info, (rgb666_color_t){63, 63, 63}, 0);
    } else {
        draw_text(320/2 - 50, 240/2 - 7, "Image not found", (rgb666_color_t){63, 0, 0}, 0);
    }

    render_button(&detail_back_button, false);
    render_button(&detail_prev_button, false);
    render_button(&detail_next_button, false);
    render_button(&detail_delete_button, false);
}

static void detail_handle_touch(touch_event_t evt) {
    if (evt.pressed_last_tick) {
        return;
    }
    if (button_hit_test(&detail_back_button, evt)) {
        detail_back_button.on_press(detail_back_button.callback_ctx);
        return;
    }
    if (button_hit_test(&detail_prev_button, evt)) {
        detail_prev_button.on_press(detail_prev_button.callback_ctx);
        return;
    }
    if (button_hit_test(&detail_next_button, evt)) {
        detail_next_button.on_press(detail_next_button.callback_ctx);
        return;
    }
    if (button_hit_test(&detail_delete_button, evt)) {
        detail_delete_button.on_press(detail_delete_button.callback_ctx);
        return;
    }
}

static void detail_handle_tick(void) {

}

const screen_t image_detail_screen = {
    .handle_tick   = &detail_handle_tick,
    .handle_touch  = &detail_handle_touch,
    .name          = "Image Detail",
    .on_enter      = &detail_on_enter,
    .on_exit       = &detail_on_exit,
    .render        = &detail_render,
};