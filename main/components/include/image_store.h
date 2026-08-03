#ifndef IMAGE_STORE_H
#define IMAGE_STORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#define IMAGE_STORE_MAX_IMAGES 200 /* cap on simultaneously stored frames */

typedef enum {
    IMAGE_FORMAT_U8_GRAY  = 0, /* raw/DF/BF/QDF capture: 1 byte/pixel */
    IMAGE_FORMAT_F32_GRAY = 1, /* DPC/QPI reconstruction: 4 bytes/pixel */
} image_format_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  format;       /* image_format_t */
    uint8_t  mode_id;      /* mode_registry acquisition mode at capture time */
    uint32_t timestamp;    /* seconds since boot, from esp_timer */
    uint32_t data_size;    /* bytes of pixel data following this header on disk */
} image_header_t;

esp_err_t image_store_init(void);

/* Saves a new frame. `data` must contain width*height*bytes_per_pixel
 * bytes matching `format`. Writes the assigned slot index to *out_index. */
esp_err_t image_store_save(const void* data, uint16_t width, uint16_t height,
                            image_format_t format, int mode_id, uint32_t* out_index);

uint32_t  image_store_count(void);

/* True if `index` currently holds a saved image (handles gaps left by
 * image_store_delete). Callers iterating the gallery should check this
 * rather than assuming indices are contiguous. */
bool image_store_exists(uint32_t index);

esp_err_t image_store_load_header(uint32_t index, image_header_t* out_header);
esp_err_t image_store_load_data(uint32_t index, void* out_buf, size_t buf_size);
esp_err_t image_store_delete(uint32_t index);

/* Loads and downsamples the stored image at `index` into a
 * thumb_w x thumb_h, 1-byte-per-pixel grayscale buffer (nearest-
 * neighbor sampled). F32 frames are normalized min/max -> 0-255.
 * Caller allocates thumb_buf (thumb_w*thumb_h bytes). */
esp_err_t image_store_load_thumbnail(uint32_t index, uint8_t* thumb_buf,
                                      uint16_t thumb_w, uint16_t thumb_h);

#endif /* IMAGE_STORE_H */