# Sprint 11 — Open-Meteo Weather Provider

**Date:** 2026-08-12
**Status:** DONE

## Goal

Replace the OpenWeather weather provider with **Open-Meteo** to eliminate the API-key/secret dependency, reduce to a single request per refresh, and include the timezone in the response — while keeping the same on-screen metrics (temperature, feels-like, humidity, pressure, wind, rain probability, condition, sunrise/sunset day/night).

## What was done

### Files modified
- `src/services/weather/weather.cpp` — replaced `OpenWeatherProvider` with `OpenMeteoProvider` (same `IWeatherService`/`WeatherData`, no interface change):
  - One request: `https://api.open-meteo.com/v1/forecast?latitude=..&longitude=..&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl&hourly=precipitation_probability&daily=sunrise,sunset&wind_speed_unit=ms&timeformat=unixtime&timezone=auto&forecast_days=1`.
  - **WMO weather-code → `weather::Condition`** mapping: `0` Clear, `1–3` Clouds, `45/48` Fog, `51–57` Drizzle, `61–67`/`80–82` Rain, `71–77`/`85–86` Snow, `95–99` Thunderstorm.
  - **Rain probability** from the current hour's `hourly.precipitation_probability` slot (the hourly array starts at local midnight, so the slot is matched by timestamp).
  - `current.time` (unix) as the observation timestamp; `daily.sunrise[0]/sunset[0]` (unix) for day/night.
  - `locationName` from the GeoIP location (`setLocation`), falling back to the configured name (Open-Meteo returns no city name).
  - Removed the API-key check and the second (forecast) request.
- `src/ui/screens/weather_screen.{h,cpp}` — added `conditionText(Condition)` returning short **localized** descriptions (Despejado/Nublado/Llovizna/Lluvia/Nieve/Niebla/Tormenta, EN or ES) and used it for the condition line (Open-Meteo only provides numeric codes).
- `src/config/weather_credentials.example.h` — marked `OPENWEATHER_API_KEY` and `WEATHER_LANG` as **legacy/unused**; kept location + timezone + UI language.
- `docs/architecture.md`, `README.md`, `docs/sprints/010-wifi-location.md` (follow-up note) — updated.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 1,070,270 B (81.7%), RAM 42,812 B (13.1%) |
| Upload | **PASS** — COM4 @ 921600 |
| Provider init | **PASS** — `initialization (Open-Meteo)` |
| Current weather | **PASS** — `temperature=31.0 C feels_like=35.4 C` |
| Wind | **PASS** — `wind=3.0 m/s dir=42 deg` (`wind_speed_unit=ms`) |
| Condition mapping | **PASS** — `condition=cloudy` (WMO code → Clouds) |
| Rain probability | **PASS** — `rain_probability=47 %` (current-hour hourly slot) |
| Sunrise/sunset | **PASS** — unix timestamps parsed (day/night) |
| Location | **PASS** — GeoIP name applied (`Merida`) |
| Refresh | **PASS** — `next refresh in 300 s` |
| Display | **PASS** — condition text, rain %, wind (km/h + cardinal), day/night background |

### Serial output (captured)
```
[WEATHER] initialization (Open-Meteo)
[WEATHER] location set: Merida (20.9666, -89.6165)
[WEATHER] request successful
[WEATHER] temperature=31.0 C feels_like=35.4 C
[WEATHER] wind=3.0 m/s dir=42 deg
[WEATHER] condition=cloudy (cloudy)
[WEATHER] rain_probability=47 % sunrise=1786534593 sunset=1786581033
[WEATHER] timestamp=1786570200
[WEATHER] next refresh in 300 s
```

## Problems and solutions

| # | Problem | Solution |
|---|---------|----------|
| 1 | ArduinoJson 7 rejected `JsonArray x = doc["hourly"]["time"];` | Use `.as<JsonArrayConst>()` for array assignment |
| 2 | Open-Meteo's hourly array starts at local midnight, so `[0]` is not the current hour | Match `current.time` to the nearest hourly slot for the rain probability |
| 3 | Open-Meteo returns numeric weather codes, no description text | WMO→`Condition` mapping in the provider; short localized text derived in the UI from `conditionId` |

## Open issues

- None. Note: values may differ slightly between providers (Open-Meteo vs OpenWeather vs other apps) — that is normal across independent data sources.
- The `OPENWEATHER_API_KEY` config is now unused (kept only as a legacy reference in the gitignored local file).

## Recommended next steps

- Sprint 12 candidates: TLS certificate hardening (replace `setInsecure()` in `http.cpp`), NVS persistence of the last-known weather, or a per-SSID manual location override.

## Notes

- **No API key required** — removes the weather secret from the project entirely.
- **One call per refresh** (288/day at a 5-min refresh) — well under Open-Meteo's free non-commercial limit (10,000/day).
- Open-Meteo returns `utc_offset_seconds` (timezone) in the response.
- No hardware changes; display config (Sprint 1 baseline) untouched.
