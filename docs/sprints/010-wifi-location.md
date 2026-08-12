# Sprint 10 — Location from Wi-Fi (GeoIP)

**Date:** 2026-08-09
**Status:** DONE

## Goal

Determine the weather location automatically from the Wi-Fi connection instead of the hard-coded Mérida coordinates, so the device can be portable between locations (home, office) without reconfiguring.

## What was done

### Files created
- `src/services/location/location.h` / `.cpp` — `location::Location { name, latitude, longitude, utcOffsetSeconds }`, `location::ILocationService` (`begin()`, `resolve(Location&)`), and a GeoIP implementation (`GeoIpLocationProvider`) with a per-SSID NVS cache.
- `docs/sprints/010-wifi-location.md`.

### Files modified
- `src/services/weather/weather.h` / `.cpp` — added `IWeatherService::setLocation(lat, lon, name)`; the provider uses the runtime location for the request URL, falling back to the compile-time `WEATHER_LATITUDE/LONGITUDE` when not set; `city.name` falls back to the set name.
- `src/ui/screens/weather_screen.h` / `.cpp` — added `setTimezoneOffsetSeconds(...)` so the "last update" time is correct in any timezone.
- `src/application/application.h` / `.cpp` — injected `location::ILocationService`; `ensureLocation()` runs before each weather fetch: on SSID change it resolves the location (cached first, GeoIP on miss), applies it to the weather service and the screen timezone; on failure it keeps the configured fallback and retries on the next refresh.
- `src/main.cpp` — wired `location::getLocationService()` into the application.
- `docs/architecture.md`, `README.md`, `docs/README.md`.

## Architecture

```
Application
   │
   ├── INetwork (SSID/connected)
   ├── IWeatherService::setLocation(lat, lon, name)
   ├── WeatherScreen::setTimezoneOffsetSeconds(...)
   └── ILocationService (resolve)
         └── GeoIpLocationProvider
              ├── per-SSID NVS cache ("loc" namespace)
              └── ipapi.co (https://ipapi.co/json/) via http::SecureClient
```

- `resolve()`: looks up the current SSID in the NVS cache (instant, no network) → miss → GeoIP lookup (lat/lon/city/`utc_offset`) → cache the result.
- Location applies **before** the weather fetch, so the first request already uses the resolved coordinates; the OpenWeather `city.name` then matches the location.
- Fallback: if GeoIP is unavailable (offline/service down), the compile-time defaults are used and the resolution retries on the next refresh (bounded by the fetch cadence).

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 1,065,904 B (81.3%), RAM 42,804 B (13.1%) |
| Upload | **PASS** — COM4 @ 921600 |
| GeoIP resolution | **PASS** — `resolving location via GeoIP (ssid=HouseMesh)` → `resolved: Merida (20.9666, -89.6165)` |
| UTC offset | **PASS** — `utc=-21600 s` (-6h, correct for Mérida) after fixing the `-0600` format |
| Weather uses resolved location | **PASS** — `location=Mérida lat=20.9666 lon=-89.6165`, `request successful` |
| Per-SSID cache | **PASS** — subsequent boots: `cached location for HouseMesh` (no API call) |
| Display | **PASS** — weather screen with resolved location; timezone applied |

## Problems and solutions

| # | Problem | Solution |
|---|---------|----------|
| 1 | `utc_offset` parsed as 0 | ipapi.co returns `-0600` (no colon); parser now accepts both `-06:00` and `-0600` |
| 2 | The stale `utc=0` persisted in the NVS cache | Temporarily cleared the "loc" cache to re-resolve; the corrected value was re-cached |
| 3 | GeoIP returns city/region accuracy | Documented: IP-based geolocation resolves the router's public IP to the ISP's city/region (fine for weather); exact per-location coordinates could use an SSID map in a future sprint |

## Open issues

- None. Note: GeoIP accuracy is city/region level (the device's public IP is the router's). If exact per-location coordinates are ever needed, a learned SSID→location map (e.g., set once via the provisioning portal) could be added.
- The `WEATHER_TIMEZONE_OFFSET_HOURS` config remains as the initial/fallback timezone; the resolved UTC offset overrides it.

## Recommended next steps

- Sprint 11 candidates: TLS certificate hardening (replace `setInsecure()`), NVS persistence of the last-known weather, or an optional per-SSID manual location override.

## Notes

- No new dependencies (reuses `http::SecureClient` and ArduinoJson; NVS via core `Preferences`).
- No hardware changes; display config (Sprint 1 baseline) untouched.

## Follow-up (post-sprint 010)

- **Day/night background + adaptive text** — the weather screen uses a sky-blue background by day and dark navy at night (from `sunrise`/`sunset`), with dark navy text on the day background and white at night (new `IDisplay::setTextColor`). The animation zone blends with the background; the moon carves with the night color.
- **7-segment temperature** — the hero temperature now uses `Font7` (native 48 px, crisp) rendered as the big number plus a small `C` unit beside it.
- **Condition line bold** — the condition description uses `TextSize::Bold` (`FreeSansBold12pt`, ~16 px) so long descriptions fit and stand out.
- **Current + forecast weather** — the provider now uses the **Current Weather API** for the observed conditions (matching other apps, and `dt` is the observation time so "Actualizado HH:MM" is correct) plus a Forecast call **only for the rain probability** (`pop`). Two requests per refresh (~192/day, within the free tier).

## Follow-up (Open-Meteo provider)

The weather provider was switched from OpenWeather to **Open-Meteo**:
- **No API key** and a single request per refresh (`api.open-meteo.com/v1/forecast`) returning current conditions, hourly rain probability, and daily sunrise/sunset (with `wind_speed_unit=ms`, `timeformat=unixtime`, `timezone=auto`).
- WMO **weather codes mapped** to `weather::Condition`; the on-screen description is now short localized text derived in the UI from `conditionId` (Despejado/Nublado/Llovizna/Lluvia/Nieve/Niebla/Tormenta).
- Rain probability comes from the **current hour's** hourly slot (matched by timestamp).
- `locationName` comes from the GeoIP location (Open-Meteo has no city name).
- `OPENWEATHER_API_KEY` / `WEATHER_LANG` config are now legacy/unused.
- One call every 5 min = 288/day (Open-Meteo free tier allows 10,000/day).
- Note: values may differ slightly between providers (Open-Meteo vs OpenWeather vs other apps) — normal across data sources.
