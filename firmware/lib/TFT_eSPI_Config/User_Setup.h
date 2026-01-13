#define USER_SETUP_INFO "BlackPill_F411_ST7789"

#define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_RGB_ORDER TFT_BGR

// STM32 SPI pins
#define TFT_MOSI PA7
#define TFT_SCLK PA5
#define TFT_CS   PA4
#define TFT_DC   PB1
#define TFT_RST  PB0

#define LOAD_GLCD
#define LOAD_FONT2

#define SPI_FREQUENCY  27000000
