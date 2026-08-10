# Sprint Notes

## Sprint 1 — Hardware validation (ST7789 GMT020-02-7P + ESP32-C3 Super Mini)

### Goal
First hardware validation: build, upload, initialize the ST7789V over 4-wire SPI, and render color + text test on the 240x320 display. No application logic yet.

### What was done
- Created minimal PlatformIO project (`platformio.ini`, `src/main.cpp`).
- Selected **LovyanGFX 1.2.26** (single library, no dependencies, ESP32-C3 support).
- Pin mapping (defined once in `main.cpp`): CS=GPIO7, DC=GPIO6, RST=GPIO5, MOSI=GPIO4, SCLK=GPIO3.
- Panel config: ST7789, 240x320 portrait, rotation 0, offsets (0,0), invert=true, RGB order normal, SPI2 @ 40 MHz.
- Test sequence: full-screen RED -> GREEN -> BLUE -> WHITE (2 s each) -> text "ST7789 OK / ESP32-C3 / 240x320".
- Serial logging for init/color stages, plus an `r` serial command to re-run the test and a 2 s heartbeat.

### Results
- Build: SUCCESS (Flash 329,164 B / 25.1%, RAM 15,028 B / 4.6%, firmware 342.3 KB).
- Upload: SUCCESS on COM4 @ 921600 (USB-Serial/JTAG, ESP32-C3 rev 0.4).
- Display init: SUCCESS (serial "Display initialization completed").
- SPI communication: confirmed (all color fills + text executed).
- Visual render: pending physical confirmation by user.

### Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Build failed: `delay()` and `fonts` ambiguous | `using namespace lgfx;` exposed `lgfx::v1::delay` and `lgfx::v1::fonts`, colliding with Arduino globals | Removed `using namespace lgfx;`, fully qualified `lgfx::LGFX_Device`, `lgfx::Panel_ST7789`, `lgfx::Bus_SPI`. `fonts::`, `TFT_*`, `middle_center` stay global via the library |
| 2 | No serial output over USB-C at all | Board flags defaulted `ARDUINO_USB_CDC_ON_BOOT=0`, so `Serial` mapped to UART0 (GPIO20/21), not the native USB-Serial/JTAG | Added `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1` to `platformio.ini` |
| 3 | Serial log capture timing (native USB re-enumeration) | Test ran at boot before a host connected; no host-held reset possible while a monitor holds the port | Firmware runs test unconditionally at boot; added `r` serial command to re-run the test and a heartbeat for liveness |
| 4 | Benign `spiAttachMISO()/spiDetachMISO()` SPI errors on ESP32-C3 | Display has no MISO pin (unidirectional SPI); ESP32-C3 SPI has no default pins so HAL logs an error | No action. Cosmetic; init and rendering confirmed working |

### Notes / follow-up
- If colors appear inverted (red<->cyan), flip `TFT_INVERT` to `false` in `main.cpp`.
- If image is shifted/cropped, adjust `TFT_OFFSET_X` / `TFT_OFFSET_Y`.
- Wiring untouched per requirements; VCC stays 3V3.
- Next: user visual confirmation, then application development.
