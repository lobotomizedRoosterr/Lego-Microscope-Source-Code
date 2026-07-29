#include "esp_err.h"
#include "esp_event.h"

esp_err_t init_computation();

extern uint8_t* computation_frame;

extern float c_scale;
extern float mult;

void begin_new_computation(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data);
void run_bf(uint8_t** data, uint8_t** buf);
void run_df(uint8_t** data, uint8_t** buf);
void run_qdf(uint8_t** data, uint8_t** buf, float c_scale);
void run_dpc_lr(uint8_t** data, uint8_t** buf);
void run_dpc_rl(uint8_t** data, uint8_t** buf);
void run_dpc_tb(uint8_t** data, uint8_t** buf);
void run_dpc_bt(uint8_t** data, uint8_t** buf);