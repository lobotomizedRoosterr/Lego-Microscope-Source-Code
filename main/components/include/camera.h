#include "esp_camera.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int capture_width;
extern int capture_height;

esp_err_t init_camera();
camera_fb_t* camera_capture_frame(void);
void camera_return_frame(camera_fb_t *fb);
camera_fb_t* camera_get_last_frame(void);

#ifdef __cplusplus
}
#endif