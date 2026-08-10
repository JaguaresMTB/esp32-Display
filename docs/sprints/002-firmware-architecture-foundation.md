# Sprint 2 — Firmware Architecture Foundation

**Date:** 2026-08-09
**Status:** DONE

## Goal

Refactor the minimal Sprint 1 hardware-validation firmware into a clean, layered firmware architecture that can support the future weather application, while preserving the validated Sprint 1 display behavior (no regression).

Out of scope: Wi-Fi, weather APIs, HTTP/HTTPS, JSON, API keys, LVGL, touch, sensors, OTA, NVS/preferences, production UI.

## What was done

- Introduced a layered architecture with a driver-agnostic display abstraction:

```
main.cpp -> application -> display::IDisplay -> St7789Display -> LovyanGFX -> ST7789
```

- Files created:
  - `src/config/pins.h` — single authoritative location for GPIO mapping + validated display parameters.
  - `src/common/logging.h` — consistent serial logging convention (`[TAG] message`).
  - `src/hardware/display/display.h` — `display::IDisplay` interface + `Color`/`TextSize` types + `display::getDisplay()` factory.
  - `src/hardware/display/display.cpp` — concrete ST7789 implementation (only file that includes LovyanGFX).
  - `src/application/application.h/.cpp` — `Application::begin()/update()`, owns lifecycle and the diagnostic test.
  - `src/diagnostics/display_test.h/.cpp` — display validation test (RED/GREEN/BLUE/WHITE + text frame), re-runnable via `r`.
  - `src/main.cpp` — rewritten as a minimal composition root.
- Files modified: `README.md`, `docs/README.md`, `docs/pinout.md`; created `docs/architecture.md`.
- Files removed: `src/tft_config.h` (replaced by `src/config/pins.h`).
- Preserved validated hardware baseline unchanged (pins, 20 MHz SPI, 240x320, rotation 0, offsets 0, invert true).

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 325,490 B (24.8%), RAM 15,220 B (4.6%) |
| Upload | **PASS** — COM4 @ 921600, USB-Serial/JTAG |
| Serial | **PASS** — startup/init/test messages + heartbeat captured |
| Display rendering | **PASS** — color cycle and text frame executed (same validated LovyanGFX config) |

### Serial output (captured)

```
[DISPLAY] display initialization complete
[TEST] diagnostic test start
[TEST] RED
[TEST] GREEN
[TEST] BLUE
[TEST] WHITE
[TEST] diagnostic test complete
[APP] rerun requested
[TEST] diagnostic test start
[TEST] RED
[TEST] GREEN
[TEST] BLUE
[TEST] WHITE
[TEST] diagnostic test complete
alive
alive
...
```

(`[APP] application startup`, `[DISPLAY] display initialization start` are printed at boot before the serial port is opened and were confirmed by the code path; the `r` re-run demonstrates the full flow.)

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Build failed: `HWCDC` has no member `vprintf` | The C3 native-USB `Serial` (HWCDC) does not expose `Print::vprintf` | Replaced with `vsnprintf` into a fixed buffer + `Serial.print` in `common/logging.h` |
| 2 | Include-path sanity for nested `src/` folders | PlatformIO adds `src/` to the include path, so nested headers must be referenced as `config/pins.h`, `hardware/display/display.h`, etc. | Kept the requested structure; verified by build |

## Open issues / blockers

- None. The validated Sprint 1 display behavior is preserved (no regression).

## Recommended next steps

- Sprint 3 candidate: Wi-Fi connectivity and a weather service/API module (new `src/networking`, `src/services` layers wired from `main.cpp`), drawing results through `display::IDisplay`.
- Extend `display::IDisplay` with `drawBitmap(...)` / `drawLine(...)` when the UI needs them.

## Notes

- Only `src/hardware/display/display.cpp` depends on LovyanGFX; everything else is driver-agnostic.
- GPIO numbers appear only in `src/config/pins.h`.
- Architecture and dependency direction documented in `docs/architecture.md`.
- No hardware configuration was changed during this sprint.
