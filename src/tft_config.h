#ifndef TFT_CONFIG_H
#define TFT_CONFIG_H

// ---- GMT020-02-7P -> ESP32-C3 Super Mini pin mapping (single source of truth) ----
// See docs/pinout.md for the full hardware reference.
#define TFT_CS    7
#define TFT_DC    6
#define TFT_RST   5
#define TFT_MOSI  4
#define TFT_SCLK  3

// ---- ST7789/ST7789V panel configuration (240x320 portrait) ----
#define TFT_WIDTH     240
#define TFT_HEIGHT    320
#define TFT_ROTATION  0
#define TFT_OFFSET_X  0
#define TFT_OFFSET_Y  0
#define TFT_INVERT    true
#define TFT_RGB_ORDER false

// ---- SPI bus ----
#define TFT_SPI_HOST   SPI2_HOST
#define TFT_SPI_MODE   0
#define TFT_FREQ_WRITE 20000000
#define TFT_FREQ_READ  16000000

#endif
