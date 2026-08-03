#include "components/include/image_store.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <math.h>

static const char* PARTITION_LABEL = "storage"; /* shared with mode_registry's config file */
static const char* MOUNT_POINT = "/storage";
static const char* TAG = "IMAGE_STORE";

static bool slot_valid[IMAGE_STORE_MAX_IMAGES];
static uint32_t stored_count = 0;
static bool initialized = false;

static void build_path(uint32_t index, char* out, size_t out_size) {
    snprintf(out, out_size, "%s/img_%05lu.bin", MOUNT_POINT, (unsigned long) index);
}

static esp_err_t ensure_spiffs_mounted(void) {
    if (esp_spiffs_mounted(PARTITION_LABEL)) {
        return ESP_OK;
    }
    esp_vfs_spiffs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = PARTITION_LABEL,
        .max_files = 8,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS partition '%s': %s",
                 PARTITION_LABEL, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t image_store_init(void) {
    esp_err_t ret = ensure_spiffs_mounted();
    if (ret != ESP_OK) {
        return ret;
    }

    memset(slot_valid, 0, sizeof(slot_valid));
    stored_count = 0;

    DIR* dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGW(TAG, "Could not open '%s' for scanning, treating as empty", MOUNT_POINT);
        initialized = true;
        return ESP_OK;
    }

    struct dirent* entry;
    unsigned long idx;
    while ((entry = readdir(dir)) != NULL) {
        if (sscanf(entry->d_name, "img_%05lu.bin", &idx) == 1) {
            if (idx < IMAGE_STORE_MAX_IMAGES) {
                slot_valid[idx] = true;
                stored_count++;
            } else {
                ESP_LOGW(TAG, "Ignoring out-of-range image file '%s'", entry->d_name);
            }
        }
    }
    closedir(dir);

    initialized = true;
    ESP_LOGI(TAG, "Found %lu existing stored image(s)", (unsigned long) stored_count);
    return ESP_OK;
}

static int find_free_slot(void) {
    for (int i = 0; i < IMAGE_STORE_MAX_IMAGES; i++) {
        if (!slot_valid[i]) {
            return i;
        }
    }
    return -1;
}

esp_err_t image_store_save(const void* data, uint16_t width, uint16_t height,
                            image_format_t format, int mode_id, uint32_t* out_index) {
    if (!initialized) {
        esp_err_t init_ret = image_store_init();
        if (init_ret != ESP_OK) {
            return init_ret;
        }
    }
    if (data == NULL || width == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int slot = find_free_slot();
    if (slot < 0) {
        ESP_LOGE(TAG, "No free image slots (max %d)", IMAGE_STORE_MAX_IMAGES);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_per_pixel = (format == IMAGE_FORMAT_F32_GRAY) ? sizeof(float) : sizeof(uint8_t);
    uint32_t data_size = (uint32_t)(width * height * bytes_per_pixel);

    image_header_t header = {
        .width = width,
        .height = height,
        .format = (uint8_t) format,
        .mode_id = (uint8_t) mode_id,
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000000ULL),
        .data_size = data_size,
    };

    char path[64];
    build_path((uint32_t) slot, path, sizeof(path));

    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open '%s' for writing", path);
        return ESP_FAIL;
    }

    bool ok = (fwrite(&header, sizeof(header), 1, f) == 1) &&
              (fwrite(data, 1, data_size, f) == data_size);
    fclose(f);

    if (!ok) {
        ESP_LOGE(TAG, "Failed writing image data to '%s'", path);
        remove(path);
        return ESP_FAIL;
    }

    slot_valid[slot] = true;
    stored_count++;

    if (out_index != NULL) {
        *out_index = (uint32_t) slot;
    }
    ESP_LOGI(TAG, "Saved image to slot %d (%ux%u, %lu bytes)", slot, width, height,
             (unsigned long) data_size);
    return ESP_OK;
}

uint32_t image_store_count(void) {
    return stored_count;
}

bool image_store_exists(uint32_t index) {
    if (index >= IMAGE_STORE_MAX_IMAGES) {
        return false;
    }
    return slot_valid[index];
}

esp_err_t image_store_load_header(uint32_t index, image_header_t* out_header) {
    if (out_header == NULL || !image_store_exists(index)) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[64];
    build_path(index, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    size_t read = fread(out_header, sizeof(image_header_t), 1, f);
    fclose(f);

    return (read == 1) ? ESP_OK : ESP_FAIL;
}

esp_err_t image_store_load_data(uint32_t index, void* out_buf, size_t buf_size) {
    if (out_buf == NULL || !image_store_exists(index)) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[64];
    build_path(index, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    image_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return ESP_FAIL;
    }
    if (header.data_size > buf_size) {
        ESP_LOGE(TAG, "Buffer too small for image %lu (%lu > %lu)",
                 (unsigned long) index, (unsigned long) header.data_size, (unsigned long) buf_size);
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t read = fread(out_buf, 1, header.data_size, f);
    fclose(f);

    return (read == header.data_size) ? ESP_OK : ESP_FAIL;
}

esp_err_t image_store_delete(uint32_t index) {
    if (!image_store_exists(index)) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[64];
    build_path(index, path, sizeof(path));

    if (remove(path) != 0) {
        ESP_LOGE(TAG, "Failed to remove '%s'", path);
        return ESP_FAIL;
    }

    slot_valid[index] = false;
    stored_count--;
    return ESP_OK;
}

esp_err_t image_store_load_thumbnail(uint32_t index, uint8_t* thumb_buf,
                                      uint16_t thumb_w, uint16_t thumb_h) {
    if (thumb_buf == NULL || thumb_w == 0 || thumb_h == 0 || !image_store_exists(index)) {
        return ESP_ERR_INVALID_ARG;
    }

    image_header_t header;
    esp_err_t ret = image_store_load_header(index, &header);
    if (ret != ESP_OK) {
        return ret;
    }

    void* full = heap_caps_malloc(header.data_size, MALLOC_CAP_SPIRAM);
    if (full == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %lu bytes for thumbnail source",
                 (unsigned long) header.data_size);
        return ESP_ERR_NO_MEM;
    }

    ret = image_store_load_data(index, full, header.data_size);
    if (ret != ESP_OK) {
        heap_caps_free(full);
        return ret;
    }

    if (header.format == IMAGE_FORMAT_U8_GRAY) {
        const uint8_t* src = (const uint8_t*) full;
        for (uint16_t ty = 0; ty < thumb_h; ty++) {
            uint32_t sy = (ty * header.height) / thumb_h;
            for (uint16_t tx = 0; tx < thumb_w; tx++) {
                uint32_t sx = (tx * header.width) / thumb_w;
                thumb_buf[ty * thumb_w + tx] = src[sy * header.width + sx];
            }
        }
    } else { /* IMAGE_FORMAT_F32_GRAY */
        const float* src = (const float*) full;
        float min_v = INFINITY, max_v = -INFINITY;
        uint32_t n = (uint32_t) header.width * header.height;
        for (uint32_t i = 0; i < n; i++) {
            if (src[i] < min_v) min_v = src[i];
            if (src[i] > max_v) max_v = src[i];
        }
        float range = (max_v - min_v);
        if (range < 1e-6f) range = 1.0f;

        for (uint16_t ty = 0; ty < thumb_h; ty++) {
            uint32_t sy = (ty * header.height) / thumb_h;
            for (uint16_t tx = 0; tx < thumb_w; tx++) {
                uint32_t sx = (tx * header.width) / thumb_w;
                float v = src[sy * header.width + sx];
                int px = (int)(((v - min_v) / range) * 255.0f);
                if (px < 0) px = 0;
                if (px > 255) px = 255;
                thumb_buf[ty * thumb_w + tx] = (uint8_t) px;
            }
        }
    }

    heap_caps_free(full);
    return ESP_OK;
}