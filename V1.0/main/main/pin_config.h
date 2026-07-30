
// camera
#define CAM_PWDN_PIN 46
#define CAM_RST_PIN 3
#define CAM_SIOD_PIN 4 //SDA
#define CAM_SIOC_PIN 5 //SCL
#define CAM_XCLK_PIN 0 //0 because current camera has onboard oscillator (no XCLK pin)

#define CAM_D0_PIN 11
#define CAM_D1_PIN 10
#define CAM_D2_PIN 9
#define CAM_D3_PIN 8
#define CAM_D4_PIN 7
#define CAM_D5_PIN 6
#define CAM_D6_PIN 2
#define CAM_D7_PIN 1

#define CAM_VSYNC_PIN 17
#define CAM_HREF_PIN 16
#define CAM_PCLK_PIN 15

//SPI3 Bus (Used by led array)
#define SPI3_MOSI_PIN 47
#define SPI3_CLK_PIN 48
#define SPI3_MISO_PIN -1 //None needed

// LED array (I am calling it MAX, for MAX7219)
#define MAX_CS_PIN 21

//SPI2 Bus (Shared by ILI9341 display and XPT2046 touch)
#define SPI2_MOSI_PIN 13 //SDI
#define SPI2_MISO_PIN 12
#define SPI2_CLK_PIN 14

//ILI9341 display
#define LCD_CS_PIN 40
#define LCD_DC_PIN 39
#define LCD_RST_PIN 38
#define LCD_BL_PIN 41 //Backlight/LED (GPIOs can output 3.3v power)

//XPT2046 touch
#define T_CS_PIN 44

