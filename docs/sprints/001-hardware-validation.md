# Sprint 1 — Hardware Validation (ST7789 GMT020-02-7P + ESP32-C3)

**Date:** 2026-08-09
**Status:** DONE — build/upload/serial confirmed; display rendering visually confirmed (see note below)

## Goal

First hardware validation of the ESP32-C3 Super Mini with the GMT020-02-7P (ST7789V, 240x320, 4-wire SPI):

1. Project builds.
2. Firmware uploads.
3. ESP32-C3 initializes the ST7789/ST7789V.
4. SPI communication works.
5. Display renders a test image (RED/GREEN/BLUE/WHITE + text).

Not in scope: Wi-Fi, BLE, LVGL, sensors, buttons, networking, storage, application logic.

## What was done

- Created minimal PlatformIO project from scratch (directory was empty).
- Selected **LovyanGFX 1.2.26** (single library, no dependencies, ESP32-C3 support).
- `platformio.ini`: espressif32 platform, Arduino framework, `esp32-c3-devkitm-1` board, `ARDUINO_USB_CDC_ON_BOOT=1` + `ARDUINO_USB_MODE=1` (native USB-Serial/JTAG serial over USB-C).
- `src/main.cpp`: display test firmware (color cycle + text frame) with serial logging, an `r` serial command to re-run the test, and a 2 s heartbeat.
- `src/tft_config.h`: single source of truth for GPIO mapping and ST7789 config.
- Pin mapping (see [docs/pinout.md](../pinout.md)): CS=GPIO7, DC=GPIO6, RST=GPIO5, MOSI=GPIO4, SCLK=GPIO3, VCC=3V3, GND=GND.
- Panel config: ST7789, 240x320 portrait, rotation 0, offsets (0,0), invert=true, RGB order normal, SPI2 @ **20 MHz** (reduced from 40 MHz for dupont-wire reliability).
- Display test sequence: RED -> GREEN -> BLUE -> WHITE (2 s each), then text frame (black background, red box, white text).
- Created documentation structure (`docs/`, README, pinout, sprint template).
- Pushed initial commit to `https://github.com/JaguaresMTB/esp32-Display`.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 329,164 B (25.1%), RAM 15,028 B (4.6%), firmware.bin ~342 KB |
| Upload | **PASS** — COM4 @ 921600, USB-Serial/JTAG, ESP32-C3 rev 0.4, XMC 4 MB flash |
| Serial (USB-C) | **PASS** — full log captured (see below) |
| Display init (software) | PASS — library reports "Display initialization completed" |
| SPI communication | Partial — commands/fills executed without error in software |
| Display rendering (visual) | **PASS** — color cycle and text frame confirmed on screen |

**Sprint close-out note:** the display is confirmed working. The blank-screen issue was resolved after verifying/reseating the SPI wiring (a loose connection on the data lines), combined with the 20 MHz SPI clock already applied in this sprint.

### Serial output (captured)

```
Display initialization completed
Color test started
RED
GREEN
BLUE
WHITE
Test text displayed
alive
alive
...
```

Note: two benign `spiAttachMISO()/spiDetachMISO()` errors appear at init. The display has no MISO pin (unidirectional SPI), so these are cosmetic.

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Build failed: `delay()` / `fonts` ambiguous | `using namespace lgfx;` exposed `lgfx::v1::delay` and `lgfx::v1::fonts`, colliding with Arduino globals | Removed `using namespace lgfx;`, fully qualified `lgfx::LGFX_Device`, `lgfx::Panel_ST7789`, `lgfx::Bus_SPI` |
| 2 | No serial output over USB-C at all | Board flags defaulted `ARDUINO_USB_CDC_ON_BOOT=0`, so `Serial` mapped to UART0 (GPIO20/21), not the native USB-Serial/JTAG | Added `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1` to `platformio.ini` |
| 3 | Serial capture timing (native USB re-enumeration) | Test ran at boot before a host connected; a host-held reset is impossible while a monitor owns the port | Firmware runs the test unconditionally at boot; added `r` serial command to re-run on demand + heartbeat for liveness |
| 4 | Benign MISO SPI errors on ESP32-C3 | Display has no MISO pin; ESP32-C3 SPI has no default pins so the HAL logs errors | No action (cosmetic) |
| 5 | **Screen showed only backlight, no image** | Backlight on = power OK; blank = data lines not reaching the panel. Resolved by verifying/reseating the SPI wiring (loose connection on the data lines) — the 20 MHz SPI clock was also applied as a mitigation | Re-seat/verify wiring; confirmed working |

## Open issues / blockers

None. Display rendering is confirmed working. Future tuning (rotation/offsets/invert) can be done as needed for the application.

## Recommended next steps

1. Proceed to application development (the validation test firmware remains isolated in this project).
2. If orientation or offsets need adjusting for the final application, tune `src/tft_config.h` (`TFT_ROTATION`, `TFT_OFFSET_X/Y`, `TFT_INVERT`).

## Notes

- Firmware text frame shows "Hello World!" (updated from the earlier test text).
- Wiring untouched per project rules; VCC stays 3.3 V.
- Repository: https://github.com/JaguaresMTB/esp32-Display (initial commit pushed).
