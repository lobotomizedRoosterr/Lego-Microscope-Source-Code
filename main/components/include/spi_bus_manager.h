#include "esp_err.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

extern spi_bus_config_t spi2_bus_config;
extern spi_bus_config_t spi3_bus_config;

esp_err_t init_spi_bus_manager(void);