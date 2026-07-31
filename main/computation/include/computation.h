#include "esp_err.h"
#include "esp_event.h"

/*initialize computation*/
esp_err_t init_computation();

/*this stores the frame of the processed microscopic image. It is greyscale image (1 byte per pixel)*/
extern float* computation_frame;

/*C_scale for QDF calculations*/
extern float c_scale;

/*
 * this is run when the frame pool finished event is called. It handles image processing
 * @param event_data this contains data passed with the event call
*/
void begin_new_computation(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_bf(uint8_t** data, float** buf);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_df(uint8_t** data, float** buf);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_qdf(uint8_t** data, float** buf, float c_scale);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_dpc_lr(uint8_t** data, float** buf);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_dpc_rl(uint8_t** data, float** buf);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_dpc_tb(uint8_t** data, float** buf);
/*
 * @param data greyscale image(s) to use in calculations
 * @param buf buffer to push processed image to.
*/
void run_dpc_bt(uint8_t** data, float** buf);
