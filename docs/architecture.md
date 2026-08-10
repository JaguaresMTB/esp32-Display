# Architecture

## Dependency direction

The firmware is layered so the application never depends on the display library internals:

```
main.cpp (composition root)
   |
   +--> application (Application)
   |        |
   |        +--> display abstraction (display::IDisplay)
   |        |        |
   |        |        v
   |        |   display implementation (St7789Display, hidden in display.cpp)
   |        |        |
   |        |        v
   |        |   LovyanGFX
   |        |        |
   |        |        v
   |        |   ST7789 panel (SPI)
   |        |
   |        +--> network abstraction (networking::INetwork)
   |        |        |
   |        |        v
   |        |   network implementation (NetworkImpl, hidden in network.cpp)
   |        |        |
   |        |        v
   |        |   Arduino WiFi (<WiFi.h>)
   |        |
   |        +--> weather abstraction (weather::IWeatherService)
   |                 |
   |                 v
   |            OpenWeatherProvider (hidden in weather.cpp)
   |                 |
   |                 v
   |            http::SecureClient (HTTPS transport, http.cpp)
   |                 |
   |                 v
   |            OpenWeather API (HTTPS)
   |
   v
diagnostics (DisplayTest) --> display::IDisplay
```

- `main.cpp` is the composition root: it wires the concrete display, network, and weather implementations (via `display::getDisplay()`, `networking::getNetwork()`, and `weather::getWeatherService()`) into the application. It contains no display-, network-, or weather-specific logic.
- The `application` layer owns the lifecycle (`begin()` / `update()`) and depends only on `display::IDisplay`, `diagnostics::DisplayTest`, `networking::INetwork`, `weather::IWeatherService`, and `ui::WeatherScreen`.
- The `diagnostics` layer (display test) depends only on `display::IDisplay`.
- Only `hardware/display/display.cpp` includes LovyanGFX. Only `networking/network.cpp` includes `<WiFi.h>`. Only `networking/http.cpp` includes `HTTPClient`/`WiFiClientSecure`. Only `services/weather/weather.cpp` knows OpenWeather details. Everything else is driver/provider-agnostic.

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

## Network abstraction

Defined in `src/networking/network.h` (`networking::INetwork`):

| Method | Purpose |
|--------|---------|
| `void begin()` | Start connecting to the configured network (non-blocking) |
| `void update()` | Advance the connection state machine (call every loop) |
| `bool isConnected()` | True when fully connected |
| `State state()` | Current state (`Disconnected`, `Connecting`, `Connected`, `Reconnecting`) |
| `const char* stateName()` | Human-readable state name |
| `String ssid()` | Connected network name |
| `String localIp()` | Assigned IPv4 address |
| `int16_t rssi()` | Signal strength in dBm |

The concrete `NetworkImpl` is hidden in `network.cpp` and exposed through `networking::getNetwork()`. Application code never calls `WiFi.*`.

### Network / application dependency

`Application` receives `networking::INetwork&` by injection (wired in `main.cpp`). `Application::update()` calls `network.update()` every loop and redraws a small Wi-Fi status screen when the state/IP/RSSI changes. The application does not block on connection: `network.begin()` returns immediately and the state machine advances from the loop.

### Wi-Fi configuration / secrets strategy

- Real credentials live only in `src/config/wifi_credentials.h`, which is **ignored by git** (see `.gitignore`).
- `src/config/wifi_credentials.example.h` is the committed template with placeholders and instructions.
- `network.cpp` includes the credentials with an `__has_include` guard plus empty-string fallback, so the build always succeeds even without the local file (empty credentials → connection is skipped and logged).
- The password is never printed by the firmware.
- To supply credentials: copy `wifi_credentials.example.h` to `wifi_credentials.h`, fill in `WIFI_SSID` and `WIFI_PASSWORD`.

### Connection state model

```
DISCONNECTED --begin()/retry--> CONNECTING --success--> CONNECTED
      ^                            |   |
      |---timeout/fail ------------   |
      |                               |
      +------ CONNECTED --drop--> RECONNECTING --retry--> CONNECTING
```

- `begin()` starts a single non-blocking attempt (`WiFi.begin`).
- `CONNECTING` polls `WiFi.status()`; success → `CONNECTED` (logs SSID/IP/RSSI); fail or timeout (~15 s) → backoff.
- `CONNECTED` detects a drop via `WiFi.status()` → `RECONNECTING`.
- `Disconnected` / `Reconnecting` wait for the next attempt time, then retry.
- All timing is elapsed-time based; no blocking loops.

### Reconnection behavior

- Backoff: 5 s, 10 s, 20 s, 40 s, capped at 60 s, growing per consecutive failure.
- `WiFi.disconnect()` is called before each attempt for a clean station state (avoids the ESP32 repeated-`WiFi.begin` failure mode).
- Consecutive-failure counter resets on a successful connection.
- The application and display remain fully responsive during connect/reconnect; the Wi-Fi status screen shows `Connecting...` / `Connected` / `No network`.

## Weather service abstraction

Defined in `src/services/weather/weather.h` (`weather::IWeatherService`):

| Method | Purpose |
|--------|---------|
| `bool begin()` | Initialize the provider (reads configuration) |
| `WeatherError getCurrentWeather(WeatherData&)` | One current-weather request (bounded); fills `data` on success |

Errors are returned as `weather::WeatherError` (`Ok`, `NotConfigured`, `NotConnected`, `HttpError`, `TlsError`, `BadStatus`, `ApiError`, `InvalidJson`, `MissingField`, `Timeout`) and converted to names with `weather::weatherErrorName()`.

### WeatherData model (provider-independent)

```cpp
struct WeatherData {
  String locationName;
  float latitude, longitude;
  float temperatureC, feelsLikeC;
  int humidityPercent, pressureHpa;
  float windSpeed;         // m/s
  int windDirection;       // degrees
  String condition;        // raw condition group, e.g. "Clouds"
  Condition conditionId;   // typed group for the UI/animation
  String conditionDescription; // localized description, e.g. "nubes dispersas"
  unsigned long timestamp; // unix seconds
};
```

`weather::Condition` is a provider-independent enum (`Clear`, `Clouds`, `Drizzle`, `Rain`, `Thunderstorm`, `Snow`, `Mist`, `Fog`, `Haze`, `Smoke`, `Dust`, `Sand`, `Ash`, `Squall`, `Tornado`, `Unknown`) mapped by the provider from the API's condition group. The UI switches on the enum, so it never depends on provider-specific vocabulary. The model intentionally stays small and never exposes provider-specific structures.

### OpenWeatherProvider

- Concrete implementation `OpenWeatherProvider` is hidden in `weather.cpp` and exposed through `weather::getWeatherService()`. Only it knows:
  - The OpenWeather URL (`api.openweathermap.org/data/2.5/weather`).
  - The request contract (`lat`, `lon`, `units=metric`, `appid`) and metric (Celsius).
  - The response JSON field names and parsing.
- The provider checks connectivity through `networking::INetwork` (via `networking::getNetwork()`) and performs the request through `http::SecureClient`; it never calls `WiFi.*` or `HTTPClient` directly.
- Missing/malformed JSON fields are handled safely (core fields are validated; optional fields default).

### HTTP/HTTPS responsibility

- `src/networking/http.h/.cpp` provides `http::SecureClient::get(url, Response&)`, a minimal HTTPS GET transport built on `WiFiClientSecure` + `HTTPClient` (Arduino core libraries).
- It returns `{ statusCode, body }`; status 0 means connection/TLS/timeout failure.
- TLS is currently used in **insecure mode** (`setInsecure()` — encrypted but no certificate verification); pinned certificates are a future hardening step.
- Application code never instantiates an HTTP client; only the weather provider does.

### Weather configuration / secrets strategy

- The OpenWeather API key and location (lat/lon/name) live only in `src/config/weather_credentials.h`, which is **ignored by git**.
- `src/config/weather_credentials.example.h` is the committed template.
- `weather.cpp` includes the credentials with an `__has_include` guard and empty-key fallback, so builds always succeed; an empty key disables weather (`NotConfigured`).
- The API key is never printed.

### Weather / application dependency

- `Application` receives `weather::IWeatherService&` by injection (wired in `main.cpp`).
- `Application::update()` performs bounded weather requests on an elapsed-time schedule (15 min after success, 5 min after failure; serial `w` forces one). It logs non-sensitive values and draws the weather screen through `ui::WeatherScreen`.
- The request never blocks indefinitely: `http::SecureClient` has a bounded timeout (~10 s), failures return immediately with a logged reason, and there is no retry loop.

## UI layer (WeatherScreen)

Defined in `src/ui/screens/weather_screen.h` (`ui::WeatherScreen`). It is a **presentation-only** component: it depends only on `display::IDisplay` and the provider-independent `weather::WeatherData`. It makes no HTTP requests, calls no WiFi APIs, parses no JSON, and knows nothing about OpenWeather or API keys.

| Method | Purpose |
|--------|---------|
| `renderLoading()` | No data yet — `Weather / Updating...` (no stale values) |
| `render(const WeatherData&)` | Full weather screen (success) |
| `renderOffline(const WeatherData&)` | Last valid data kept + `Wi-Fi offline` + last update time |
| `renderUpdateFailed(const WeatherData&)` | Last valid data kept + `Update failed` + last update time |
| `updateAnimation(now)` | Called every loop; throttled (~12 fps); redraws the animation zone |

Screen layout (240x320 portrait):
- Colored header bar with the location name (accent-folded to ASCII, title case).
- **Animation zone** (y 36-140) — condition-based scene, or spinner while loading.
- Hero temperature (XLarge), feels-like, condition.
- Separator line.
- Metrics (humidity / wind / direction) left/right aligned.
- Footer with last-update local time (`HH:MM`), or the offline/update-failed indicator.

The header bar color encodes the state (blue = ready, yellow = offline, red = update failed); all text stays white. The location name is transliterated (e.g. `Mérida` → `Merida`) because the built-in fonts are ASCII-only.

### Weather animation

`updateAnimation()` is time-driven from `millis()` (no timers, no framebuffer): it clears the animation zone and redraws a small procedural scene. Scenes map from `WeatherData.conditionId`:

| Condition group | Scene |
|-----------------|-------|
| Clear | Sun with rotating rays |
| Clouds | Drifting cloud shapes |
| Drizzle, Rain | Cloud + falling rain lines |
| Thunderstorm | Cloud + blinking lightning bolt |
| Snow | Cloud + falling snowflakes |
| Mist, Fog, Haze, Smoke, Dust, Sand, Ash | Drifting fog bands |
| Squall, Tornado, Unknown | Clouds (fallback) |
| (loading) | Spinning dots |

Scenes are drawn with `fillCircle`/`fillEllipse`/`fillTriangle`/`drawLine`/`fillRect` through `IDisplay` — no image assets, no LovyanGFX types in the UI layer. Only the animation zone is redrawn each frame (static text renders only on state change), keeping the screen stable and the loop responsive.

### Language (UI labels + descriptions)

- UI labels are selectable via `WEATHER_UI_LANG` in the weather config (`0` = English, `1` = Spanish). `Application` passes a `ui::Language` to the screen; labels use ASCII-safe Spanish (`Sensacion`, `Humedad`, `Viento`, `Direccion`, `Actualizado`, `Wi-Fi sin conexion`, `Error de actualizacion`).
- Weather descriptions come from the API already localized via `WEATHER_LANG` (e.g. `es` → `nubes dispersas`), so no translation table is needed in firmware.

### Display abstraction extensions

`display::IDisplay` was extended across sprints:
- `TextSize::XLarge` — scaled large font (used for the hero temperature).
- `TextAlign { Left, Center, Right }` + `drawTextAligned(text, x, y, size, align)`.
- `drawLine`, `fillCircle`, `fillEllipse`, `fillTriangle` — primitives for the weather animation scenes.
- `Color` extended with `Orange`, `Gray`.

No LovyanGFX types are exposed through the UI layer.

## Application responsibilities

`Application` owns:
- Network state tracking (via `networking::INetwork`).
- **WeatherData caching** — `_weatherData` + `_hasWeatherData`. The cache is replaced **only** after a successful request (`WeatherError::Ok`); failures keep the last valid data (RAM-only; lost on reboot).
- **Refresh scheduling** — elapsed-time based, never `delay()`:
  - `kWeatherRefreshIntervalMs = 15 min` after a successful request.
  - `kWeatherRetryIntervalMs = 5 min` after a failed request (bounded, no tight loop).
  - Serial command `w` forces an immediate refresh; `r` re-runs the display diagnostic.
- **State selection** — `Loading` (no data), `Ready` (data + connected + last fetch ok), `Offline` (data + not connected), `UpdateFailed` (data + connected + last fetch failed). The screen is redrawn only when the state or the data timestamp changes.
- **Animation** — `_weatherScreen.updateAnimation(millis())` runs every loop iteration; the screen throttles internally (~12 fps).

The main loop stays responsive: Wi-Fi, refresh scheduling, and rendering are all driven from `Application::update()` without blocking loops.

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

Tags in use: `APP` (application), `DISPLAY` (display init), `NET` (networking), `WEATHER` (weather service), `TEST` (diagnostics).

## Diagnostic test

`src/diagnostics/display_test.h/.cpp` contains the hardware validation test (RED, GREEN, BLUE, WHITE, text frame). It is retained for future hardware troubleshooting and can be re-run with the `r` serial command. It uses only `display::IDisplay`, so it works against any display driver.

## Future extension points

- **Weather UI polish:** more scenes/icons, richer animations, custom fonts — all still presentation-only through `IDisplay`.
- **Forecast / more fields:** extend `WeatherData` and the provider parser; the model stays provider-independent.
- **Persistent cache:** move the last-known weather into NVS so it survives reboot (currently RAM-only).
- **HTTP hardening:** replace `setInsecure()` with pinned/root-CA certificate verification.
- **Application UI:** add more screens in the `ui` layer; they draw through `IDisplay`.
- **Bitmap/images:** extend `IDisplay` with `drawBitmap(...)`.
- **NVS/settings:** a future `config/settings` module; pins/display params remain compile-time constants in `config/pins.h`.
