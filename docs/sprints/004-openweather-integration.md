# Sprint 4 — OpenWeather Integration

**Date:** 2026-08-09
**Status:** DONE

## Goal

Establish the weather data pipeline: Wi-Fi -> HTTPS -> OpenWeather -> OpenWeatherProvider -> WeatherData -> Application, with the TFT remaining functional. Provider-isolated weather service; no production weather UI yet.

## What was done

### Files created
- `src/services/weather/weather.h` — `weather::WeatherData`, `weather::IWeatherService`, `weather::WeatherError` enum, `weather::getWeatherService()`.
- `src/services/weather/weather.cpp` — `OpenWeatherProvider` (hidden concrete class) + factory; request construction and JSON parsing.
- `src/networking/http.h` / `http.cpp` — `http::SecureClient` HTTPS GET transport over `WiFiClientSecure` + `HTTPClient`.
- `src/config/weather_credentials.example.h` — committed template (API key placeholder + Mérida, Mexico coordinates).
- `src/config/weather_credentials.h` — local, gitignored real API key + coordinates.

### Files modified
- `src/main.cpp` — wired `weather::getWeatherService()` into the application (composition root).
- `src/application/application.h/.cpp` — injected `weather::IWeatherService`; one-shot weather request when Wi-Fi first becomes `Connected`; logging of all non-sensitive values; simple `Weather OK / <temp> C` diagnostic screen.
- `platformio.ini` — added dependency `bblanchon/ArduinoJson@^7.0.0`.
- `.gitignore` — added `src/config/weather_credentials.h`.
- `README.md`, `docs/README.md`, `docs/architecture.md` — updated.

### Weather architecture

```
Application -> weather::IWeatherService -> OpenWeatherProvider -> http::SecureClient -> OpenWeather API (HTTPS)
```

Only `weather.cpp` knows OpenWeather URLs, JSON fields, and the API key. Only `http.cpp` touches `HTTPClient`/`WiFiClientSecure`. The application never sees the API key, URLs, or JSON.

### WeatherData model

`locationName`, `latitude`, `longitude`, `temperatureC`, `feelsLikeC`, `humidityPercent`, `pressureHpa`, `windSpeed` (m/s), `windDirection` (deg), `condition`, `conditionDescription`, `timestamp` (unix s). Provider-independent, intentionally small.

### IWeatherService API

`bool begin()`; `WeatherError getCurrentWeather(WeatherData&)`. `WeatherError`: `Ok`, `NotConfigured`, `NotConnected`, `HttpError`, `TlsError`, `BadStatus`, `ApiError`, `InvalidJson`, `MissingField`, `Timeout`.

### OpenWeatherProvider

- Request: `https://api.openweathermap.org/data/2.5/weather?lat=..&lon=..&units=metric&appid=KEY` (lat/lon, metric/Celsius, no city search).
- Connectivity checked through `networking::INetwork::isConnected()`; transport through `http::SecureClient`.
- JSON parsed with ArduinoJson; core fields validated (`main.temp`, `main.feels_like`, `main.humidity`, `name`); optional fields default; error envelope (`cod != 200`) handled.

### HTTP/HTTPS

`http::SecureClient` uses `WiFiClientSecure` + `HTTPClient` (Arduino core, no new library), bounded timeout (~10 s), returns `{ statusCode, body }`. TLS in **insecure mode** (`setInsecure()`) — validation build; cert pinning deferred.

### Configuration / secrets

Gitignored `weather_credentials.h` (API key + coordinates); committed `.example.h` template; `__has_include` guard with empty-key fallback so builds always succeed; key never printed.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 981,802 B (74.9% of 1.3 MB), RAM 41,268 B (12.6%) |
| Upload | **PASS** — COM4 @ 921600 |
| Serial | **PASS** — full `[WEATHER]` logging captured |
| Wi-Fi | **PASS** — HouseMesh, 192.168.68.114 |
| HTTPS | **PASS** — TLS GET to api.openweathermap.org succeeded |
| OpenWeather request | **PASS** — HTTP 200 |
| JSON parsing | **PASS** — parsed successfully |
| WeatherData | **PASS** — all fields populated |
| Display | **PASS** — diagnostic test + `Weather OK / 25.4 C` screen |

### Serial output (TEST 1 — successful request)
```
[NET] connected
[NET] ssid=HouseMesh ip=192.168.68.114 rssi=-60 dBm
[WEATHER] requesting current weather
[WEATHER] request successful
[WEATHER] location=Mérida lat=20.9756 lon=-89.6170
[WEATHER] temperature=25.4 C feels_like=26.0 C
[WEATHER] humidity=75 % pressure=1017 hPa
[WEATHER] wind=1.7 m/s dir=94 deg
[WEATHER] condition=Clouds (broken clouds)
[WEATHER] timestamp=1786335066
```

### TEST 2 — Wi-Fi unavailable (PASS)
Empty Wi-Fi credentials → `[NET] no credentials configured; skipping connection`; weather never requested (guarded by `isConnected()`); heartbeat continues; display shows `No network`. No hang.

### TEST 3 — Invalid API key (PASS)
```
[WEATHER] request failed: HTTP status 401
[WEATHER] invalid API key (401)
[WEATHER] request failed: API_ERROR
```
Heartbeat continues; no crash; no retry loop.

### TEST 4 — Display regression (PASS)
Boot diagnostic test runs; `r` re-runs RED/GREEN/BLUE/WHITE + text frame.

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | None blocking — pipeline worked on first live test | — | — |
| 2 | Location name prints as `M??rida` in the serial capture | UTF-8 accented char from OpenWeather decoded by the capture terminal as non-UTF-8 | Cosmetic capture artifact only; data/display correct |

## Open issues / blockers

- None.
- Note: TLS is used in insecure mode (no certificate verification). Acceptable for validation; pinning is a recommended future hardening step.

## Recommended next steps

- Sprint 5 candidate: production weather UI screen (location, temperature, condition, humidity/wind) drawn through `display::IDisplay`, plus an elapsed-time periodic refresh (e.g. every 10-30 min) in `Application::update()`.
- Hardening: replace `setInsecure()` with pinned/root-CA certificate verification.

## Notes

- Only dependency added: ArduinoJson (HTTPClient/WiFiClientSecure are Arduino core libraries).
- `src/config/weather_credentials.h` is gitignored; the real API key was never committed or logged.
- Weather request is one-shot (no periodic refresh) per this sprint's scope.
