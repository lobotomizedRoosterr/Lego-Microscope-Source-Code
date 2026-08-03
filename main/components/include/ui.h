#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "components/include/render_engine.h"
#include "esp_err.h"

/* ============================================================
 * Color
 * ============================================================ */

/* ============================================================
 * Shared low-level structs (composable across all widgets)
 * ============================================================ */

enum v_alignment {
    V_ALIGNMENT_TOP,
    V_ALIGNMENT_MIDDLE,
    V_ALIGNMENT_BOTTOM,
};
enum h_alignment {
    H_ALIGNMENT_LEFT,
    H_ALIGNMENT_CENTER,
    H_ALIGNMENT_RIGHT
};

/* Bounding box. Used for both drawing and (by default) touch hit-testing. */
typedef struct {
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
} rect_t;

typedef struct {
    const uint8_t* raw;
    uint16_t w;
    uint16_t h;
    uint8_t h_align;
    uint8_t v_align;
} icon_t;

/* Foreground/background pair, with a second pair for the "selected"
 * or "active" state. Reused by buttons, toggles, list items, etc. */
typedef struct {
    rgb666_color_t fg;
    rgb666_color_t bg;
} color_pair_t;

typedef struct {
    color_pair_t normal;
    color_pair_t selected;
} widget_colors_t;

typedef struct {
    rgb666_color_t color;
    uint8_t         width;   /* pixels, 0 = no border */
} border_style_t;

/* Forward-declared; defined wherever the 8x14 bitmap font lives
 * (e.g. font.h). Kept opaque here so ui.h doesn't need to know
 * the font's internal layout. */
typedef struct font_s font_t;

typedef struct {
    const font_t   *font;
    rgb666_color_t  color;
    uint8_t v_align;
    uint8_t h_align;
} text_style_t;

/* ============================================================
 * Touch
 * ============================================================ */

typedef struct {
    int16_t x;
    int16_t y;
    bool    pressed;
    bool pressed_last_tick;
} touch_event_t;

/* ============================================================
 * Widgets
 * ============================================================ */

typedef void (*button_callback_t)(void *ctx);

typedef struct {
    rect_t           bounds;
    rect_t           hit_area;   /* optional larger tap target; if w/h == 0, bounds is used */
    widget_colors_t  colors;
    border_style_t   border;
    text_style_t     text_style;
    icon_t           icon;
    const char      *label;
    button_callback_t on_press;
    void            *callback_ctx;
} button_t;

typedef struct {
    rect_t           bounds;
    widget_colors_t  colors;
    bool             value;
} toggle_t;

typedef struct {
    rect_t           bounds;
    widget_colors_t  colors;
    int16_t          min;
    int16_t          max;
    int16_t          value;
} slider_t;

/* One tile in the gallery's paged thumbnail grid. */
typedef struct {
    rect_t   bounds;
    uint32_t image_index;   /* index into image_store */
} grid_cell_t;

/* ============================================================
 * Screens
 * ============================================================ */

typedef struct screen_s {
    const char *name;
    void (*on_enter)(void);
    void (*on_exit)(void);
    void (*render)(void);
    void (*handle_touch)(touch_event_t evt);
    void (*handle_tick)(void);   /* optional; NULL if screen has no live updates */
} screen_t;

#define SCREEN_STACK_MAX_DEPTH 6

typedef struct {
    const screen_t *stack[SCREEN_STACK_MAX_DEPTH];
    uint8_t         depth;
} screen_manager_t;

/* Screen manager API */
esp_err_t screen_manager_init(screen_manager_t *mgr, const screen_t *initial_screen);
void screen_push(screen_manager_t *mgr, const screen_t *screen);
void screen_pop(screen_manager_t *mgr);
void screen_replace(screen_manager_t *mgr, const screen_t *screen);
const screen_t *screen_current(const screen_manager_t *mgr);

/* Dispatch helpers, called from the main loop */
void screen_manager_render(screen_manager_t *mgr);
void screen_manager_handle_touch(screen_manager_t *mgr, touch_event_t evt);
void screen_manager_tick(screen_manager_t *mgr);

/* ============================================================
 * Shared rendering / hit-test helpers (implemented in ui.c,
 * built on top of render_engine.c primitives)
 * ============================================================ */

void render_rect_border(rect_t bounds, border_style_t border);
bool hit_test_rect(rect_t bounds, int16_t touch_x, int16_t touch_y);

void render_button(const button_t *btn, bool is_selected);
bool button_hit_test(const button_t *btn, touch_event_t evt);

void render_toggle(const toggle_t *tog);
void render_slider(const slider_t *sld);
void render_grid_cell(const grid_cell_t *cell, bool is_selected);

void ui_navigate_push(const screen_t *screen);
void ui_navigate_pop(void);
void ui_navigate_replace(const screen_t *screen);

void ui_task(void* arg);

/* Declared screens (one instance per screen, defined in their own .c files) */
extern const screen_t live_view_screen;
extern const screen_t gallery_screen;
extern const screen_t image_detail_screen;
extern const screen_t settings_screen;
extern const screen_t mode_debug_screen;
extern const screen_t directory_screen;

#endif /* UI_H */