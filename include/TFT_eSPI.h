#define USER_SETUP_INFO "MH_ET_Lolin_TFT14"

#define ST7735_DRIVER    // Define strictly for Lolin 1.4 TFT

#define TFT_WIDTH  128
#define TFT_HEIGHT 128
#define ST7735_GREENTAB128 // Specific to 1.44" Lolin Shield

#define TFT_MISO -1 // Not used
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

#define LOAD_GLCD   // Font 1
#define LOAD_FONT2  // Font 2
#define LOAD_FONT4  // Font 4
#define LOAD_FONT6  // Font 6
#define LOAD_FONT7  // Font 7
#define LOAD_FONT8  // Font 8
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000 // ST7735 handles this
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000