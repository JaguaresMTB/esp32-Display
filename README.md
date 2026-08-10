# esp32-Display

Hardware validation and development project for an **ESP32-C3 Super Mini** driving a **GMT020-02-7P** 2.0" **240x320** display (ST7789/ST7789V) over 4-wire SPI.

## Status

| Area | Status |
|------|--------|
| Build | PASS |
| Upload | PASS |
| Serial (USB-C) | PASS |
| Display init (software) | PASS |
| Display rendering (visual) | **PASS** — color cycle and text confirmed on screen |

See [Sprint 1 close-out](docs/sprints/001-hardware-validation.md) for details.

## Hardware

- MCU: ESP32-C3 Super Mini
- Display: GMT020-02-7P, 2.0", 240x320, ST7789/ST7789V, 4-wire SPI, board rev VER:1.3
- Connection: USB-C (native USB-Serial/JTAG)

Full wiring reference: [docs/pinout.md](docs/pinout.md)

## Quick start

Requirements: PlatformIO Core (`pio`).

```sh
pio run -t upload              # build and flash to the detected port
pio device monitor -b 115200   # serial output
```

Upload target (native USB-Serial/JTAG):

```ini
; platformio.ini
board = esp32-c3-devkitm-1
upload_port = COM4
```

### Serial control

- On boot the firmware runs the display test automatically.
- Send `r` over serial to re-run the test on demand.
- The firmware prints an `alive` heartbeat every 2 s.

### Display test sequence

1. Full-screen RED (2 s)
2. Full-screen GREEN (2 s)
3. Full-screen BLUE (2 s)
4. Full-screen WHITE (2 s)
5. Text frame: black background, red box, white text

## Project structure

```
esp32-Display/
├── platformio.ini
├── README.md
├── src/
│   ├── main.cpp          # test firmware
│   └── tft_config.h      # pin mapping + display config (single source of truth)
└── docs/
    ├── README.md         # documentation index & conventions
    ├── pinout.md         # hardware wiring reference
    └── sprints/          # one document per completed sprint
        ├── 000-TEMPLATE.md
        └── 001-hardware-validation.md
```

## Configuration

All display GPIO assignments and ST7789 panel parameters live in a single file: [`src/tft_config.h`](src/tft_config.h). Do not scatter GPIO numbers elsewhere.

## Documentation convention

Every sprint is documented when considered done (see [docs/README.md](docs/README.md) and the [sprint template](docs/sprints/000-TEMPLATE.md)). Sprint documents include objectives, results, problems found and how they were solved, and open issues.
