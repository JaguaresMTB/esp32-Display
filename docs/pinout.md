# Pinout — GMT020-02-7P ↔ ESP32-C3 Super Mini

This is the single source of truth for the physical wiring. **Do not modify the wiring without updating this file.**

## Wiring table

| GMT020-02-7P (display) | ESP32-C3 Super Mini | Notes |
|------------------------|---------------------|-------|
| CS  | GPIO7  | SPI chip select |
| DC  | GPIO6  | SPI data/command |
| RST | GPIO5  | Reset (active low) |
| SDA | GPIO4  | **SPI MOSI** (NOT I2C) |
| SCL | GPIO3  | **SPI clock** (NOT I2C) |
| VCC | 3V3    | **3.3 V only — never 5 V** |
| GND | GND    | Common ground |

## Hardware facts

- Interface: 4-wire SPI (no MISO — the display has no readback pin).
- The 7-pin version has **no separate backlight (BL) pin**; backlight is tied to VCC.
- Physical resolution: 240x320. Controller: ST7789/ST7789V. Board revision: VER:1.3.

## Firmware reference

The same mapping lives in firmware at `src/tft_config.h`:

```c
#define TFT_CS    7
#define TFT_DC    6
#define TFT_RST   5
#define TFT_MOSI  4
#define TFT_SCLK  3
```

If you change the wiring, update **both** this file and `src/tft_config.h`.
