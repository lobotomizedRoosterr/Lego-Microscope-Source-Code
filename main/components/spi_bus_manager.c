#include "esp_err.h"
#include "components/include/spi_bus_manager.h"
#include "main/pin_config.h"

#include "components/include/display.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include"esp_log.h"

static const char* TAG = "SPI_BUS_MANAGER";


spi_bus_config_t spi2_bus_config; /*configuration for spi2 bus*/
spi_bus_config_t spi3_bus_config; /*config for spi3 bus*/


esp_err_t init_spi_bus_manager(void) {
    esp_err_t ret = ESP_OK;

    spi2_bus_config = (spi_bus_config_t) {
        .mosi_io_num     = SPI2_MOSI_PIN,
        .miso_io_num     = SPI2_MISO_PIN,
        .sclk_io_num     = SPI2_CLK_PIN,
        .quadwp_io_num   = -1, // no write protect
        .quadhd_io_num   = -1, //no hold signal
        .max_transfer_sz = (DISPLAY_WIDTH * DISPLAY_HEIGHT) * sizeof(uint16_t), //maximum transfer size, set to max for what will be sent to the lcd
    };

    ret = spi_bus_initialize(SPI2_HOST, &spi2_bus_config, SPI_DMA_CH_AUTO); // init spi bus 2 SPI_DMA_CH_AUTO enables DMA, and uses a driver specified DMA channel function
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi2_bus_initialize (LCD/Touch) failed: %s", esp_err_to_name(ret));
        return ret;
    }
    // SPI 3 bus config
    spi3_bus_config  = (spi_bus_config_t){
        .mosi_io_num     = SPI3_MOSI_PIN,
        .miso_io_num     = SPI3_MISO_PIN,
        .sclk_io_num     = SPI3_CLK_PIN,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 8, // max transfer size in bytes
    };

    ret = spi_bus_initialize(SPI3_HOST, &spi3_bus_config, SPI_DMA_CH_AUTO); // init SPI2 3 bus with DMA enabled
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi3_bus_initialize (LED Matrix) failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ret;
}
