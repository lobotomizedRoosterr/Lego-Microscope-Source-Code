#include "esp_err.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

/*this is the configuration used to define the spi2 bus for the driver*/
extern spi_bus_config_t spi2_bus_config;
/*this is the configuration used to define the spi3 bus for the driver*/
extern spi_bus_config_t spi3_bus_config;

/*initialize spi busses*/
esp_err_t init_spi_bus_manager(void);
