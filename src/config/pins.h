#pragma once

// ---- Hardware GPIO mapping (validated Sprint 1) ----
// GMT020-02-7P -> ESP32-C3 Super Mini. See docs/pinout.md.
#define TFT_CS    7
#define TFT_DC    6
#define TFT_RST   5
#define TFT_MOSI  4
#define TFT_SCLK  3

// ---- Display hardware parameters (validated Sprint 1) ----
// Do not change without a documented reason (see docs/architecture.md).
#define TFT_WIDTH      240
#define TFT_HEIGHT     320
#define TFT_ROTATION   0
#define TFT_OFFSET_X   0
#define TFT_OFFSET_Y   0
#define TFT_INVERT     true
#define TFT_RGB_ORDER  false
#define TFT_SPI_MODE   0
#define TFT_FREQ_WRITE 20000000
#define TFT_FREQ_READ  16000000
