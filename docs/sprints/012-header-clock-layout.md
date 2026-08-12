# Sprint 12 — Header Clock and Header Layout

**Date:** 2026-08-12
**Status:** DONE

## Goal

Add a live current-time clock to the weather screen header (updating every minute) and refine the header layout: bigger clock font, city name at the top-left with a location-pin icon, time at the top-right.

## What was done

### Files modified
- `src/application/application.cpp` — after the location/timezone is resolved (`ensureLocation()`), call `configTime(utcOffsetSeconds, 0, "pool.ntp.org")` (ESP32 core SNTP; fallback uses the configured timezone offset) so the device keeps accurate local time; call `_weatherScreen.updateClock()` every loop.
- `src/ui/screens/weather_screen.{h,cpp}` —
  - New `updateClock()`: reads `time(NULL)` (NTP-synced), applies the UTC offset, formats **`hh:mm am/pm`** (12-hour, zero-padded, e.g. `03:44 pm`), and redraws only the top-right header region when the minute changes. Uses `TextSize::Metric` (~20 px) and the stored header-bar color.
  - Header layout: **city name moved to the top-left** (left-aligned) with a **drawn location-pin icon** before it; the clock sits at the **top-right**; the time was bumped from `Small` to `Metric`.
  - Fine-tuning: city name 2 px lower and 3 px left of the initial position.

### Behavior
- The clock only appears on the weather data screen (Ready/Offline/UpdateFailed); boot checklist and provisioning screens are unchanged.
- Before NTP syncs, the clock is skipped (`time(NULL) < 1e6`).
- The clock updates on minute boundaries without a full redraw.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 1,073,868 B (81.9%), RAM 42,868 B (13.1%) |
| Upload | **PASS** — COM4 @ 921600 |
| NTP sync | **PASS** — `configTime` called with the resolved UTC offset (-21600 s) |
| Clock | **PASS** — renders `hh:mm am/pm` at the top-right, updates each minute (visually confirmed) |
| Header layout | **PASS** — location pin + city at top-left, time top-right |
| Weather | **PASS** — `request successful`, `condition=cloudy`, refresh every 5 min |

## Problems and solutions

| # | Problem | Solution |
|---|---------|----------|
| 1 | Device has no real-time clock | NTP via `configTime` (ESP32 core, no library); timezone offset already known from GeoIP |
| 2 | Redrawing the whole header every minute would be wasteful | `updateClock()` redraws only the top-right region when the minute changes |
| 3 | City/time overlap in the 30 px header | City left-aligned (with pin), time right-aligned; cleared region sized to fit the 20 px time |

## Open issues

- None. The clock depends on NTP (internet); before sync or when offline it is simply not drawn.

## Recommended next steps

- Sprint 13 candidates: TLS certificate hardening (replace `setInsecure()` in `http.cpp`), NVS persistence of the last-known weather, or a per-SSID manual location override.

## Notes

- No new dependencies (SNTP is part of the ESP32 Arduino core).
- The clock uses the resolved local timezone offset (GeoIP), falling back to the configured offset.
- No hardware changes; display config (Sprint 1 baseline) untouched.

## Follow-up (post-sprint 012)

- **Footer time uses the real NTP fetch time** — the "Actualizado" footer previously showed Open-Meteo's observation timestamp, which only advances every 15 minutes (`current.interval = 900`), making the footer look frozen despite 5-min refreshes. Now `Application::fetchWeather()` records `time(NULL)` (NTP) on success (falling back to the API timestamp until NTP syncs), so the footer advances with every refresh.
- **12-hour footer format** — `formatTime()` now outputs `hh:mm am/pm` (e.g., `Actualizado 04:18 pm`), matching the header clock.
- **Pressure fix** — `pressure_msl` is read as a float (`| 0.0f`) before casting to int; previously parsed as 0.
