#include "esp_camera.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*width in px of images to be captured*/
extern int capture_width;
/*height in px of images to be captured*/
extern int capture_height;

/*initialize camera*/
esp_err_t init_camera();
/*capture a frame
 * @returns returns camera_fb_t which contains frame buffer for camera. It comes from the esp-cam driver
*/
camera_fb_t* camera_capture_frame(void);
/*
 * Return a camera frame. If a frame is captured, it must be returned, or another cannot be retrieved.
 * @param fb camera frame buffer to be returned
 * 
*/
void camera_return_frame(camera_fb_t *fb);

/*
 * returns last captured frame
*/
camera_fb_t* camera_get_last_frame(void);

#ifdef __cplusplus
}
#endif
