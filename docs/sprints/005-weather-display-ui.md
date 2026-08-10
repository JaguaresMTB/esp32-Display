# Sprint 5 — Weather Display UI and Periodic Refresh

**Date:** 2026-08-09
**Status:** DONE

## Goal

Create the first production weather screen and implement periodic weather refresh, while preserving all prior functionality (hardware, architecture, Wi-Fi, OpenWeather pipeline, display diagnostics). No LVGL or GUI framework.

## What was done

### Files created
- `src/ui/screens/weather_screen.h` / `.cpp` — presentation-only `ui::WeatherScreen` depending only on `display::IDisplay` and `weather::WeatherData`.

### Files modified
- `src/hardware/display/display.h` / `.cpp` — extended `IDisplay` minimally: `TextSize::XLarge`, `TextAlign` + `drawTextAligned(...)`, `drawLine(...)`.
- `src/application/application.h` / `.cpp` — weather-data cache (`_weatherData`, `_hasWeatherData`), elapsed-time refresh scheduling (15 min / 5 min), UI state selection (`Loading`/`Ready`/`Offline`/`UpdateFailed`), serial `w` (force refresh) and `r` (diagnostic) commands; removed the old generic Wi-Fi status screen (state now conveyed by the weather screen).
- `src/config/weather_credentials.example.h` / `weather_credentials.h` — added `WEATHER_TIMEZONE_OFFSET_HOURS` (-6 for Mérida, Mexico).
- `README.md`, `docs/README.md`, `docs/architecture.md` — updated.

### UI architecture

```
Application -> ui::WeatherScreen -> display::IDisplay
```

`WeatherScreen` makes no HTTP requests, calls no WiFi APIs, parses no JSON, and knows nothing about OpenWeather or API keys. It only renders `WeatherData`.

### WeatherScreen

| Method | Purpose |
|--------|---------|
| `renderLoading()` | No data — `Weather / Updating...` (no stale values) |
| `render(data)` | Full weather screen |
| `renderOffline(data)` | Last valid data + `Wi-Fi offline` + last update |
| `renderUpdateFailed(data)` | Last valid data + `Update failed` + last update |

Layout: colored header (location, accent-folded to ASCII), hero temperature (XLarge), feels-like, condition, separator line, left/right-aligned metrics (Humidity / Wind / Direction), footer with last-update local time. State color: blue = ready, yellow = offline, red = update failed.

### State management / refresh

- Cache (`_weatherData`) replaced **only** on success; failures keep the last valid data (RAM-only, documented).
- Scheduling is elapsed-time based (no `delay()`): 15 min after success, 5 min after failure (bounded, no tight loop).
- States: `Loading` (no data), `Ready`, `Offline` (not connected), `UpdateFailed` (fetch failed). Screen redrawn only on state/timestamp change.
- Serial `w` forces an immediate refresh; `r` re-runs the display diagnostic and then restores the weather screen.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 984,532 B (75.1% of 1.3 MB), RAM 41,348 B (12.6%) |
| Upload | **PASS** — COM4 @ 921600 |
| Serial | **PASS** — full `[WEATHER]` + refresh logs |
| Wi-Fi | **PASS** — HouseMesh, 192.168.68.114 |
| OpenWeather | **PASS** — HTTPS 200, JSON parsed |
| WeatherData | **PASS** — 25.4 C, feels 26.0 C, 75 %, 1017 hPa, 1.7 m/s, Clouds/broken clouds |
| WeatherScreen | **PASS** — all states render; data visible |
| Refresh behavior | **PASS** — `next refresh in 900 s`, `w` forces immediate request |
| Display | **PASS** — weather screen + diagnostic regression |

### TEST 1 — Successful weather display (PASS)
```
[WEATHER] request successful
[WEATHER] temperature=25.4 C feels_like=26.0 C
[WEATHER] humidity=75 % pressure=1017 hPa
[WEATHER] wind=1.7 m/s dir=94 deg
[WEATHER] condition=Clouds (broken clouds)
[WEATHER] next refresh in 900 s
```
Screen shows: MERIDA / 25.4 C / Feels like 26.0 C / Broken clouds / Humidity 75% / Wind 1.7 m/s / Direction 94 deg / Updated HH:MM.

### TEST 2 — Wi-Fi unavailable (PASS)
After a successful fetch, powering off the AP produced `[NET] disconnected`; the screen switched to the Offline state **keeping the last valid weather data** (`Wi-Fi offline | HH:MM`), heartbeat continued (fully responsive), and the device auto-reconnected and returned to Ready when the AP came back.

### TEST 3 — Weather API failure (PASS)
Invalid API key → `HTTP status 401` → `API_ERROR (next in 300 s)`; heartbeat continued; no crash; bounded 5-min retry (no tight loop). With no prior data the screen stayed in Loading (no stale values); the failure is logged.

### TEST 4 — Periodic refresh (PASS)
Serial `w` forced an immediate weather request (`[WEATHER] weather refresh requested` → new successful request → `next refresh in 900 s`), confirming the refresh path re-requests and re-renders without waiting 15 minutes.

### TEST 5 — Display diagnostic regression (PASS)
`r` re-runs RED/GREEN/BLUE/WHITE + text frame; the weather screen redraws afterwards.

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Accented location `Mérida` would render incorrectly | Built-in LovyanGFX fonts are ASCII-only (no é / °) | Added an ASCII accent-folding + title-case helper in `WeatherScreen` (`Mérida` → `Merida`) |
| 2 | No large font with letters for the hero temperature | Font6/Font8 are numbers-only; Font4 (26 px) is the largest ASCII font | Added `TextSize::XLarge` mapping to Font4 scaled 2x (~52 px) |
| 3 | Degree symbol `°` unavailable in built-in fonts | ASCII-only glyph set | Temperature rendered as `25.4 C` (ASCII-safe) |

## Open issues / blockers

- Weather cache is RAM-only; the last valid data does not survive a reboot (NVS persistence is a future improvement).
- The "Update failed with retained data" path is exercised structurally (cache replaced only on success) and via the offline test, but not with a live in-session API failure (the key is compile-time); TEST 3 covers the no-data failure case.
- The last-update time uses `WeatherData.timestamp` (OpenWeather observation time) with a fixed UTC offset from config; no NTP/clock, no DST handling. Documented limitation.

## Recommended next steps

- Sprint 6 candidates: NVS persistence of the last-known weather, weather icon glyphs/drawing, or TLS certificate hardening.
- Optionally reduce heartbeat log noise when the UI is the primary output.

## Notes

- No new dependencies added (ArduinoJson from Sprint 4 reused; HTTPClient/WiFiClientSecure are core libs).
- The production interval stays 15 minutes; the `w` serial command is the dev/test shortcut (production constant unchanged).
- No hardware changes; display config (Sprint 1 baseline) untouched.
