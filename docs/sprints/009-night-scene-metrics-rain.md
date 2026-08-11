# Sprint 9 — Night Scene, Metric Units, and Rain Probability

**Date:** 2026-08-09
**Status:** DONE

## Goal

Improve the weather screen: show a moon instead of a sun on clear nights, use metric-friendly units (wind in km/h), and replace the wind-direction row with a real rain probability.

## What was done

### Files modified
- `src/services/weather/weather.h` — added to `WeatherData`: `rainProbabilityPercent`, `sunrise`, `sunset` (kept `windDirection`, no longer displayed).
- `src/services/weather/weather.cpp` — switched from the Current Weather endpoint to the **Forecast API** (`/data/2.5/forecast` with `cnt=1`); parse from `list[0]` (temp/feels/humidity/pressure/wind/weather, **`pop` → rain probability %**) and `city` (name, coords, **sunrise/sunset**).
- `src/ui/screens/weather_screen.h` / `.cpp` — new `Scene::Moon`; `sceneFor(condition, isNight)` picks Sun by day / Moon by night for Clear; `isNight()` from `timestamp` vs `sunrise`/`sunset`; new `drawMoon()` (crescent + twinkling stars); metrics row: wind in **km/h** (`windSpeed × 3.6`) and **Direccion → Lluvia** (rain probability %).
- `src/application/application.cpp` — extended weather log with `rain_probability` and `sunrise/sunset`.

### Behavior
- **Clear + day** → sun (unchanged); **Clear + night** → crescent moon + twinkling stars. Other conditions unchanged at night (clouds/rain/etc. look the same).
- **Rain probability** is real: from the Forecast API `pop` (probability of precipitation for the current 3-hour window), shown as `Lluvia <n> %`.
- **Wind** shown in km/h (`Viento 20.9 km/h`). Temperature stays Celsius; humidity %, pressure hPa (all metric).

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** |
| Upload | **PASS** — COM4 @ 921600 (after a USB re-plug; the port briefly dropped) |
| Forecast API | **PASS** — `request successful` |
| Rain probability | **PASS** — `rain_probability=0 %` parsed |
| Sunrise/sunset | **PASS** — `sunrise=1786448165 sunset=1786494684` parsed |
| Localized description | **PASS** — `condition=Clouds (nubes)` still localized (`lang=es`) |
| Wind km/h | **PASS** — 5.8 m/s → `Viento 20.9 km/h` |
| Night moon scene | **PASS** — verified on-device (temporary test command forced night-clear; crescent moon + stars confirmed, then removed) |

## Problems and solutions

| # | Problem | Solution |
|---|---------|----------|
| 1 | Current Weather API has no rain probability (`pop`) | Switched to the Forecast API (`cnt=1`); temp/wind/etc. come from the current 3-hour window |
| 2 | Device's USB port dropped mid-upload (COM4 gone) | User re-plugged the cable; upload re-attempted successfully |
| 3 | Verifying the night scene requires clear skies at night | Temporary `m` serial command forced night-clear for visual verification, then removed |

## Open issues

- None. Note: with the Forecast endpoint, temperature/wind/etc. reflect the current 3-hour forecast window rather than the instantaneous observation.

## Follow-up (wind cardinal direction)

After the sprint, the wind metric was updated to append the cardinal direction from the parsed `windDirection` degrees (8-point compass: N, NE, E, SE, S, SW, W, NW), e.g. `Viento 20.9 km/h NE`. Implemented via `WeatherScreen::windCardinal()`; the `Direccion` row remains replaced by `Lluvia %`.

## Recommended next steps

- Sprint 10 candidates: TLS certificate hardening, NVS persistence of last-known weather, or a general night dark theme for all scenes.

## Notes

- No config/dependency changes; same API key.
- No hardware changes; display config (Sprint 1 baseline) untouched.
