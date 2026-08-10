# Architecture

## Dependency direction

The firmware is layered so the application never depends on the display library internals:

```
main.cpp (composition root)
   |
   v
application (Application)
   |
   v
display abstraction (display::IDisplay)
   |
   v
display implementation (St7789Display, hidden in display.cpp)
   |
   v
LovyanGFX
   |
   v
ST7789 panel (SPI)
```

- `main.cpp` is the composition root: it wires the concrete display implementation (via `display::getDisplay()`) into the application and the diagnostic test. It contains no display-specific logic.
- The `application` layer owns the lifecycle (`begin()` / `update()`) and depends only on `display::IDisplay` and `diagnostics::DisplayTest`.
- The `diagnostics` layer (display test) also depends only on `display::IDisplay`.
- Only `hardware/display/display.cpp` includes LovyanGFX. Everything else is driver-agnostic.

## Display abstraction

Defined in `src/hardware/display/display.h` (`display::IDisplay`):

| Method | Purpose |
|--------|---------|
| `bool init()` | Initialize the display hardware |
| `void clear()` | Clear to black |
| `void fillScreen(Color)` | Solid color over the whole display |
| `void fillRect(x, y, w, h, Color)` | Solid filled rectangle |
| `void drawText(text, cx, cy, TextSize)` | Text centered at a point |
| `int32_t width()` / `height()` | Display dimensions |

Supporting types (driver-independent):

- `display::Color` — 16-bit 565 color enum (`Red`, `Green`, `Blue`, `White`, `Black`, `Cyan`, `Magenta`, `Yellow`).
- `display::TextSize` — `Small`, `Large` (maps to library fonts internally).

Future primitives (e.g. `drawBitmap(...)`, `drawLine(...)`) are added to `IDisplay` when the application needs them; the implementation adds the matching LovyanGFX call.

The concrete class (`St7789Display`) is kept inside `display.cpp` and exposed only through `display::getDisplay()`, so callers never see LovyanGFX types.

## Hardware configuration ownership

`src/config/pins.h` is the single authoritative location for hardware definitions:

- GPIO mapping: `TFT_CS`, `TFT_DC`, `TFT_RST`, `TFT_MOSI`, `TFT_SCLK`.
- Validated display parameters: resolution (240x320), rotation (0), offsets (0,0), inversion, RGB order, SPI mode and frequencies.

These values are the validated Sprint 1 baseline. They must not be changed without a documented reason. Only `hardware/display/display.cpp` consumes them.

## Logging convention

`src/common/logging.h` provides a single helper:

```cpp
logging::begin(115200);
logging::info("TAG", "message");          // [TAG] message
logging::info("TAG", "value=%d", n);      // [TAG] value=42
```

Tags in use: `APP` (application), `DISPLAY` (display init), `TEST` (diagnostics).

## Diagnostic test

`src/diagnostics/display_test.h/.cpp` contains the hardware validation test (RED, GREEN, BLUE, WHITE, text frame). It is retained for future hardware troubleshooting and can be re-run with the `r` serial command. It uses only `display::IDisplay`, so it works against any display driver.

## Future extension points

- **Wi-Fi / weather service:** add new layers (e.g. `networking`, `services`) under `src/` that depend on `application` or are wired from `main.cpp`. The display stays behind `IDisplay`.
- **Application UI:** add views/widgets in the `application` layer; they draw through `IDisplay`.
- **Bitmap/images:** extend `IDisplay` with `drawBitmap(...)`.
- **NVS/settings:** a future `config/settings` module; pins/display params remain compile-time constants in `config/pins.h`.
