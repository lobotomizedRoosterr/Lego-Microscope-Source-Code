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
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "components/include/ui.h"
 
#define UI_TASK_PERIOD_MS 33   /* ~30 fps; raise if touch feels laggy, lower if CPU-bound */
 
static screen_manager_t g_screen_manager;

int selected_screen;

void render_rect_border(rect_t bounds, border_style_t border)
{
    if (border.width == 0) {
        return;
    }
    for (uint8_t i = 0; i < border.width; i++) {
        draw_rect(bounds.x + i, bounds.y + i,
                  bounds.w - (2 * i), bounds.h - (2 * i),
                  border.color);
    }
}
 
bool hit_test_rect(rect_t bounds, int16_t touch_x, int16_t touch_y)
{
    return touch_x >= bounds.x &&
           touch_x <  bounds.x + (int16_t)bounds.w &&
           touch_y >= bounds.y &&
           touch_y <  bounds.y + (int16_t)bounds.h;
}
 
void render_button(const button_t *btn, bool is_selected)
{
    color_pair_t colors = is_selected ? btn->colors.selected : btn->colors.normal;
 
    fill_rect(btn->bounds.x, btn->bounds.y, btn->bounds.w, btn->bounds.h,
               colors.bg);
 

    if(btn->icon.raw != NULL) {


        uint16_t img_x = 0;
        uint16_t img_y = 0;
        switch(btn->icon.h_align) {
            case H_ALIGNMENT_CENTER:
                img_x = btn->bounds.x + (btn->bounds.w - btn->icon.w)/2;
                break;
            case H_ALIGNMENT_LEFT:
                img_x = btn->bounds.x;
                break;
            case H_ALIGNMENT_RIGHT:
                img_x = btn->bounds.x + btn->bounds.w-btn->icon.w;
                break;
        }
        
        switch(btn->icon.v_align) {
            case V_ALIGNMENT_MIDDLE:
                img_y = btn->bounds.y + (btn->bounds.h - btn->icon.h)/2;
                break;
            case V_ALIGNMENT_TOP:
                img_y = btn->bounds.y;
                break;
            case V_ALIGNMENT_BOTTOM:
                img_y = btn->bounds.y + btn->bounds.h-btn->icon.h;
                break;
        }
        draw_image(img_x, img_y, btn->icon.raw, btn->icon.w, btn->icon.h);
    }
 
    render_rect_border(btn->bounds, btn->border);
    if (btn->label != NULL) {
        /* Centering left as a simple heuristic: assumes 8px-wide glyphs,
           swap for real text-width measurement if labels vary a lot. */
        uint16_t text_w = 0;
        for (const char *p = btn->label; *p != '\0'; p++) {
            text_w += 8;
        }

        int16_t text_x = 0;
        int16_t text_y = 0;
        switch(btn->text_style.h_align) {
            case H_ALIGNMENT_CENTER:
                text_x = btn->bounds.x + (btn->bounds.w - text_w) / 2;
                break;
            case H_ALIGNMENT_LEFT:
                text_x = btn->bounds.x + btn->border.width;
                break;
            case H_ALIGNMENT_RIGHT:
                text_x = btn->bounds.x+btn->bounds.w-btn->border.width;
                break;
        }
        //TODO : Config minus for bottom to be for each font
        switch(btn->text_style.v_align) {
            case V_ALIGNMENT_MIDDLE:
                text_y = btn->bounds.y + (btn->bounds.h - 14) / 2;
                break;
            case V_ALIGNMENT_TOP:
                text_y = btn->bounds.y+btn->border.width;
                break;
            case V_ALIGNMENT_BOTTOM:
                text_y = btn->bounds.y+btn->bounds.h-btn->border.width-14;
                break;
        }
        
 
        draw_text( text_x, text_y, btn->label,
                  btn->text_style.color, 0);
    }
}
 
bool button_hit_test(const button_t *btn, touch_event_t evt)
{
    if (!evt.pressed) {
        return false;
    }
    rect_t area = (btn->hit_area.w != 0 && btn->hit_area.h != 0)
                      ? btn->hit_area
                      : btn->bounds;
    return hit_test_rect(area, evt.x, evt.y);
}
 
void render_toggle(const toggle_t *tog)
{
    color_pair_t colors = tog->value ? tog->colors.selected : tog->colors.normal;
    fill_rect(tog->bounds.x, tog->bounds.y, tog->bounds.w, tog->bounds.h,
               colors.bg);
}
 
void render_slider(const slider_t *sld)
{
    /* Track */
    fill_rect(sld->bounds.x, sld->bounds.y, sld->bounds.w, sld->bounds.h,
               sld->colors.normal.bg);
 
    /* Fill proportional to value within [min, max] */
    if (sld->max > sld->min) {
        int32_t range   = sld->max - sld->min;
        int32_t clamped = sld->value < sld->min ? sld->min :
                           (sld->value > sld->max ? sld->max : sld->value);
        uint16_t fill_w = (uint16_t)(((clamped - sld->min) * sld->bounds.w) / range);
 
        fill_rect(sld->bounds.x, sld->bounds.y, fill_w, sld->bounds.h,
                   sld->colors.selected.bg);
    }
}
 
void render_grid_cell(const grid_cell_t *cell, bool is_selected)
{
    /* Thumbnail bitmap itself is drawn by the gallery screen (it owns
       image_store access); this just draws the selection highlight. */
    if (is_selected) {
        border_style_t highlight = {
            .color = (rgb666_color_t){ .r = 63, .g = 63, .b = 0 },
            .width = 2,
        };
        render_rect_border(cell->bounds, highlight);
    }
}

esp_err_t screen_manager_init(screen_manager_t *mgr, const screen_t *initial_screen)
{
    esp_err_t ret = ESP_OK;
    mgr->depth = 0;
    mgr->stack[mgr->depth++] = initial_screen;
    if (initial_screen->on_enter != NULL) {
        initial_screen->on_enter();
    }
    return ret;
}
void ui_navigate_push(const screen_t *screen)    { 
    screen_push(&g_screen_manager, screen); 
}
void ui_navigate_pop(void)                       { 
    screen_pop(&g_screen_manager); 
}
void ui_navigate_replace(const screen_t *screen) { 
    screen_replace(&g_screen_manager, screen); 
}
void screen_push(screen_manager_t *mgr, const screen_t *screen)
{
    if (mgr->depth >= SCREEN_STACK_MAX_DEPTH) {
        return;   /* stack full; drop rather than corrupt memory */
    }
 
    const screen_t *prev = mgr->stack[mgr->depth - 1];
    if (prev->on_exit != NULL) {
        prev->on_exit();
    }
 
    mgr->stack[mgr->depth++] = screen;
 
    if (screen->on_enter != NULL) {
        screen->on_enter();
    }
}
 
void screen_pop(screen_manager_t *mgr)
{
    if (mgr->depth <= 1) {
        return;   /* never pop the root screen */
    }
 
    const screen_t *prev = mgr->stack[mgr->depth - 1];
    if (prev->on_exit != NULL) {
        prev->on_exit();
    }
 
    mgr->depth--;
 
    const screen_t *now = mgr->stack[mgr->depth - 1];
    if (now->on_enter != NULL) {
        now->on_enter();
    }
}
 
void screen_replace(screen_manager_t *mgr, const screen_t *screen)
{
    const screen_t *prev = mgr->stack[mgr->depth - 1];
    if (prev->on_exit != NULL) {
        prev->on_exit();
    }
 
    mgr->stack[mgr->depth - 1] = screen;
 
    if (screen->on_enter != NULL) {
        screen->on_enter();
    }
}
 
const screen_t *screen_current(const screen_manager_t *mgr)
{
    return mgr->stack[mgr->depth - 1];
}
 
void screen_manager_render(screen_manager_t *mgr)
{
    const screen_t *screen = screen_current(mgr);
    if (screen->render != NULL) {
        screen->render();
    }
}
 
void screen_manager_handle_touch(screen_manager_t *mgr, touch_event_t evt)
{
    const screen_t *screen = screen_current(mgr);
    if (screen->handle_touch != NULL) {
        screen->handle_touch(evt);
    }
}
 
void screen_manager_tick(screen_manager_t *mgr)
{
    const screen_t *screen = screen_current(mgr);
    if (screen->handle_tick != NULL) {
        screen->handle_tick();
    }
}

 
/* ============================================================
 * ui_task
 *
 * Owns the screen manager and drives touch -> tick -> render ->
 * push_frame each loop iteration. Runs on its own core, separate
 * from image acquisition, so no acquisition-side locking is
 * needed here beyond whatever frame_pool/image_store already do
 * internally.
 * ============================================================ */


 
void ui_task(void *arg)
{
    (void)arg;
 
    screen_manager_init(&g_screen_manager, &live_view_screen);
    screen_manager_init(&g_screen_manager, &mode_debug_screen);
    screen_manager_init(&g_screen_manager, &settings_screen);
    screen_manager_init(&g_screen_manager, &gallery_screen);
    screen_manager_init(&g_screen_manager, &directory_screen);

    ui_navigate_replace(&directory_screen);
    ui_navigate_push(&live_view_screen);
    
    TickType_t last_wake = xTaskGetTickCount();
    bool last_tick_pressed = false;
 
    for (;;) {
        touch_data td = get_touch();
        touch_event_t evt = {
            .x = td.touch_x,
            .y = td.touch_y,
            .pressed = td.pressed,
            .pressed_last_tick=last_tick_pressed
        };
        last_tick_pressed=evt.pressed;
 
        if (evt.pressed) {
            screen_manager_handle_touch(&g_screen_manager, evt);
        }
 
        screen_manager_tick(&g_screen_manager);
        screen_manager_render(&g_screen_manager);
        push_frame();

 
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(UI_TASK_PERIOD_MS));
    }
}