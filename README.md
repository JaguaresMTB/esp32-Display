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
| Firmware architecture | **PASS** — layered: main -> application -> display/network/weather abstractions |
| Wi-Fi connection | **PASS** — connects to HouseMesh (192.168.68.114), auto-reconnects |
| OpenWeather | **PASS** — HTTPS request, JSON parse, WeatherData populated (25.4 C in Mérida) |
| Weather UI + refresh | **PASS** — production weather screen, 15-min periodic refresh, offline/failure states |
| Weather animation | **PASS** — condition-based animated scenes (sun/clouds/rain/storm/snow/fog) |
| Language | **PASS** — Spanish UI labels + localized descriptions (`lang=es`) |

See [Sprint 1](docs/sprints/001-hardware-validation.md), [Sprint 2](docs/sprints/002-firmware-architecture-foundation.md), [Sprint 3](docs/sprints/003-wifi-foundation.md), [Sprint 4](docs/sprints/004-openweather-integration.md), [Sprint 5](docs/sprints/005-weather-display-ui.md), and [Sprint 6 — Weather Animation and Spanish UI](docs/sprints/006-weather-animation-i18n.md).

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

- On boot the firmware initializes, connects to Wi-Fi and fetches weather (no automatic color test).
- Send `r` over serial to run the display diagnostic test (RED/GREEN/BLUE/WHITE) on demand.
- Send `w` over serial to force an immediate weather refresh (default interval is 15 min).
- Send `p` over serial to enter Wi-Fi provisioning mode (same as the BOOT long-press).
- The firmware prints an `alive` heartbeat every 2 s.

### Boot sequence

1. Display initializes; the persistent boot log is dumped to serial.
2. Screen shows a **step-by-step checklist** (no animation):
   - `Wi-Fi`: Conectando → Autorizando → Conectado ✓
   - `Clima`: Conectando → Autorizando → Actualizado ✓
   - Every row shows its attempt number.
3. Once weather data arrives, the full weather screen (with condition animation) appears.
4. A persistent boot/error log (last 10 events) survives unplug/replug and is printed at every boot.

The display diagnostic test (RED/GREEN/BLUE/WHITE) runs only via `r`.

### Display test sequence

1. Full-screen RED (2 s)
2. Full-screen GREEN (2 s)
3. Full-screen BLUE (2 s)
4. Full-screen WHITE (2 s)
5. Text frame: black background, red box, white text

### Wi-Fi setup (on-device provisioning)

Wi-Fi is configured **on the device** — no computer or reflash needed.

**First use (no credentials):**
1. Power the device — it automatically enters Wi-Fi Setup mode.
2. Connect your phone/PC to the **`WeatherDisplay-XXXX`** network (open).
3. Open **`http://192.168.4.1`**.
4. Select or type your Wi-Fi **SSID**, enter the **password**, tap **Save & Connect**.
5. The device validates the connection, saves it, and starts showing the weather.

**Reconfiguration (device already configured):**
1. Hold the **BOOT** button ~3 s (the device enters Wi-Fi Setup mode).
2. Connect to **`WeatherDisplay-XXXX`**, open `http://192.168.4.1`.
3. Configure the new network. It becomes active **only after the connection is validated**.
   - If the new credentials are wrong, the portal shows an error and your previous configuration remains intact — just retry.

> A temporary Wi-Fi outage does **not** erase your credentials or enter setup mode automatically — the device retries/reconnects on its own.

### OpenWeather setup

OpenWeather API key + location are provided via a local, git-ignored file:

```sh
cp src/config/weather_credentials.example.h src/config/weather_credentials.h
# edit src/config/weather_credentials.h -> set OPENWEATHER_API_KEY, coordinates,
# WEATHER_LANG (es/en) and WEATHER_UI_LANG (0=English, 1=Spanish)
```

`src/config/weather_credentials.h` is never committed and the API key is never logged. After Wi-Fi connects, the application performs one weather request and logs the result (temperature, humidity, pressure, wind, condition). The display shows an animated condition scene and periodic refreshes run every 15 minutes.

## Project structure

```
esp32-Display/
├── platformio.ini
├── README.md
├── src/
│   ├── main.cpp                  # minimal composition root (wiring only)
│   ├── config/
│   │   ├── pins.h                # GPIO mapping + display params (single source of truth)
│   │   ├── wifi_credentials.example.h  # LEGACY (superseded by provisioning)
│   │   ├── wifi_credentials.h    # legacy compile-time creds (LOCAL, gitignored, unused)
│   │   ├── weather_credentials.example.h # OpenWeather key + location template (tracked)
│   │   └── weather_credentials.h # real key + coordinates (LOCAL, gitignored)
│   ├── common/
│   │   ├── logging.h             # serial logging convention
│   │   └── error_log.h/.cpp      # persistent boot/error log (NVS)
│   ├── hardware/
│   │   └── display/
│   │       ├── display.h         # display abstraction (IDisplay)
│   │       └── display.cpp       # LovyanGFX/ST7789 implementation
│   ├── networking/
│   │   ├── network.h             # Wi-Fi abstraction (INetwork)
│   │   ├── network.cpp           # network manager (normal + provisioning modes)
│   │   ├── wifi_credentials.h/.cpp # credential model + NVS store
│   │   ├── provisioning.h/.cpp   # SoftAP + DNS + HTTP provisioning portal
│   │   ├── http.h                # HTTPS GET transport (http::SecureClient)
│   │   └── http.cpp              # WiFiClientSecure + HTTPClient implementation
│   ├── services/
│   │   └── weather/
│   │       ├── weather.h         # weather abstraction (IWeatherService, WeatherData)
│   │       └── weather.cpp       # OpenWeatherProvider + JSON parsing
│   ├── ui/
│   │   └── screens/
│   │       ├── weather_screen.h  # production weather screen (presentation only)
│   │       └── weather_screen.cpp
│   ├── application/
│   │   ├── application.h         # application lifecycle
│   │   └── application.cpp
│   └── diagnostics/
│       ├── display_test.h        # display validation test
│       └── display_test.cpp
└── docs/
    ├── README.md                 # documentation index & conventions
    ├── architecture.md           # architecture & dependency direction
    ├── pinout.md                 # hardware wiring reference
    └── sprints/                  # one document per completed sprint
        ├── 000-TEMPLATE.md
        ├── 001-hardware-validation.md
        ├── 002-firmware-architecture-foundation.md
        ├── 003-wifi-foundation.md
        └── 004-openweather-integration.md
```

## Configuration

All display GPIO assignments and ST7789 panel parameters live in a single file: [`src/config/pins.h`](src/config/pins.h). Do not scatter GPIO numbers elsewhere.

## Architecture

See [docs/architecture.md](docs/architecture.md) for the dependency direction, the display abstraction API, hardware configuration ownership, and future extension points.

## Documentation convention

Every sprint is documented when considered done (see [docs/README.md](docs/README.md) and the [sprint template](docs/sprints/000-TEMPLATE.md)). Sprint documents include objectives, results, problems found and how they were solved, and open issues.
