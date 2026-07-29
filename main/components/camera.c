#include "components/include/camera.h"
#include "main/app_state.h"
#include "main/pin_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_task_wdt.h"
static const char *TAG = "CAMERA";
static camera_fb_t *last_frame = NULL;

int capture_width;
int capture_height;
 
/* =========================
   INTERNAL RESET SEQUENCE
   ========================= */
static void camera_hardware_reset(void)
{
    gpio_set_direction(CAM_PWDN_PIN,  GPIO_MODE_OUTPUT);
    gpio_set_direction(CAM_RST_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(CAM_PWDN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(CAM_PWDN_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(CAM_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(CAM_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}
 
/* =========================
   INIT
   ========================= */
esp_err_t init_camera()
{

    capture_width = 320;
    capture_height = 240;
 
    camera_hardware_reset();
 
    camera_config_t config = {
        .pin_pwdn  = CAM_PWDN_PIN,
        .pin_reset = CAM_RST_PIN,
 
        .pin_xclk     = CAM_XCLK_PIN,
        .xclk_freq_hz = 20000000,

        .pin_sccb_sda = CAM_SIOD_PIN,
        .pin_sccb_scl = CAM_SIOC_PIN,
 
        .pin_d0 = CAM_D0_PIN,
        .pin_d1 = CAM_D1_PIN,
        .pin_d2 = CAM_D2_PIN,
        .pin_d3 = CAM_D3_PIN,
        .pin_d4 = CAM_D4_PIN,
        .pin_d5 = CAM_D5_PIN,
        .pin_d6 = CAM_D6_PIN,
        .pin_d7 = CAM_D7_PIN,
 
        .pin_vsync = CAM_VSYNC_PIN,
        .pin_href  = CAM_HREF_PIN,
        .pin_pclk  = CAM_PCLK_PIN,
 
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
 
        .pixel_format = PIXFORMAT_GRAYSCALE,
 
        .frame_size   = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
 
        .fb_count    = 2,
        .grab_mode   = CAMERA_GRAB_LATEST,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .sccb_i2c_port = -1,  // let driver create its own port
    };
    gpio_set_pull_mode(CAM_SIOD_PIN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(CAM_SIOC_PIN, GPIO_PULLUP_ONLY);
 
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
        return err;
    }
 
    sensor_t *s = esp_camera_sensor_get();
    
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_exposure_ctrl(s, 0);
    s->set_gain_ctrl(s, 0);
    s->set_whitebal(s, 0);
    s->set_awb_gain(s, 0);

    s->set_raw_gma(s, 0);
    s->set_lenc(s, 0);
    s->set_sharpness(s, -2); //-2 to +2

    s->set_aec_value(s, 100);   // out of 1200
    s->set_agc_gain(s, 0);      // 0 to 30
 
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}
 
/* =========================
   FRAME CAPTURE
   ========================= */
camera_fb_t* camera_capture_frame(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
 
    if (!fb) {
        ESP_LOGE(TAG, "Frame capture failed (timeout or no stream)");
        return NULL;
    }
 
    last_frame = fb;
    return fb;
}
 
/* =========================
   RETURN FRAME
   ========================= */
void camera_return_frame(camera_fb_t *fb)
{
    if (!fb) return;
 
    esp_camera_fb_return(fb);
 
    if (last_frame == fb) {
        last_frame = NULL;
    }
}
 
/* =========================
   LAST FRAME ACCESS
   ========================= */
camera_fb_t* camera_get_last_frame(void)
{
    return last_frame;
}