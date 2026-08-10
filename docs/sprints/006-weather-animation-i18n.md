# Sprint 6 — Weather Animation and Spanish UI

**Date:** 2026-08-09
**Status:** DONE

## Goal

Add a weather-condition animation to the weather screen (procedural, native to `IDisplay`) and make the UI switchable to Spanish (labels + localized weather descriptions). No new GUI frameworks or image assets.

## What was done

### Files created
- `docs/sprints/006-weather-animation-i18n.md`

### Files modified
- `src/services/weather/weather.h` / `.cpp` — added `weather::Condition` enum + `WeatherData.conditionId` (provider-independent, mapped from the API condition group); added `&lang=` to the request URL.
- `src/config/weather_credentials.example.h` / `weather_credentials.h` — added `WEATHER_LANG` (`es`) and `WEATHER_UI_LANG` (`1` = Spanish).
- `src/hardware/display/display.h` / `.cpp` — added `fillCircle`, `fillEllipse`, `fillTriangle`; extended `Color` with `Orange`, `Gray`.
- `src/ui/screens/weather_screen.h` / `.cpp` — scene-at-top layout with an animation zone; condition-based animated scenes; spinner for loading; `ui::Language` + selectable Spanish labels; accent-folding/title-case text helpers.
- `src/application/application.h` / `.cpp` — drives `_weatherScreen.updateAnimation(millis())` each loop; passes `ui::Language` from config.
- `docs/architecture.md`, `README.md`, `docs/README.md` — updated.

### Weather animation

- `WeatherScreen::updateAnimation(now)` runs every loop, throttles internally (~80 ms ≈ 12 fps), clears the animation zone (y 36-140) and redraws a small procedural scene. Static text renders only on state change; the loop stays responsive.
- Scenes map from `WeatherData.conditionId`:
  - Clear → sun with rotating rays
  - Clouds → drifting cloud shapes
  - Drizzle/Rain → cloud + falling rain lines
  - Thunderstorm → cloud + blinking lightning bolt
  - Snow → cloud + falling snowflakes
  - Mist/Fog/Haze/Smoke/Dust/Sand/Ash → drifting fog bands
  - Squall/Tornado/Unknown → clouds (fallback)
  - Loading → spinning dots
- Scenes use `fillCircle`/`fillEllipse`/`fillTriangle`/`drawLine`/`fillRect` via `IDisplay`. No image assets, no framebuffer, no LovyanGFX types in the UI layer.

### Spanish UI

- UI labels selectable via `WEATHER_UI_LANG` (`0` English, `1` Spanish): `Sensacion`, `Humedad`, `Viento`, `Direccion`, `Actualizado`, `Wi-Fi sin conexion`, `Error de actualizacion`, `Actualizando...`, `Clima`.
- Weather descriptions arrive localized from OpenWeather via `WEATHER_LANG=es` (e.g. `nubes dispersas`, `muy nuboso`) — no translation table in firmware.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 995,324 B (75.9% of 1.3 MB), RAM 41,364 B (12.6%) |
| Upload | **PASS** — COM4 @ 921600 |
| Serial | **PASS** — Spanish description logged (`condition=Clouds (muy nuboso)`) |
| Wi-Fi | **PASS** — HouseMesh, 192.168.68.114 |
| OpenWeather | **PASS** — HTTPS 200, `lang=es` works |
| WeatherData | **PASS** — `conditionId` mapped (Clouds) |
| WeatherScreen | **PASS** — scene-at-top layout + Spanish labels |
| Animation | **PASS** — all 6 scenes visually validated on the display |
| Display | **PASS** — no regression (boot test, `r`, `w` work) |

### Scene validation
All scenes were exercised on the live display via a temporary `c` serial command that cycled `conditionId` through Clear, Clouds, Rain, Thunderstorm, Snow, Fog. Each animation rendered correctly (confirmed visually). The temporary command was removed afterwards; production code is clean.

### Transient network notes
During the session, OpenWeather requests intermittently failed with `start_ssl_client: -1` / DNS failures. These were transient mesh-AP/network issues (not related to the animation): refreshes succeeded both before and after those moments, and the failure handling (bounded retry, retained data, no crash) worked as designed.

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Accented text (`Mérida`, `Dirección`) won't render in built-in fonts | LovyanGFX built-in fonts are ASCII-only | Accent-folding + title-case helper in `WeatherScreen`; ASCII-safe Spanish labels |
| 2 | No large letter font for the hero temperature | Font6/Font8 are numbers-only | `TextSize::XLarge` = Font4 scaled 2x |
| 3 | UI would need to match OpenWeather's English condition strings | Provider vocabulary leaking to the UI | Added provider-independent `weather::Condition` enum mapped in the provider |
| 4 | Weather descriptions should be Spanish | Would need a translation table | Use OpenWeather `lang=es` so descriptions arrive localized |

## Open issues / blockers

- None.
- Note: built-in fonts are ASCII-only, so Spanish labels use unaccented ASCII (`Direccion`, `Actualizacion`).

## Follow-up fix (loading-screen feedback + first-retry speed)

After the sprint, the device appeared "stuck" on the loading screen when the mesh AP's internet was down (repeated DNS/TLS failures). Root cause: the loading screen gave no feedback and the first fetch only retried every 5 minutes.

Fixes (in `application` + `weather_screen`):
- `renderLoading(bool offline)` — shows `Sin conexion / Wi-Fi no disponible` (yellow header) when Wi-Fi is unavailable, `Actualizando...` otherwise. Re-renders when connectivity changes.
- `kInitialRetryIntervalMs = 30 s` — while no weather data exists, failed fetches retry every 30 s (so the device recovers quickly once the network returns); after data exists, the 5-min failure cadence applies.

Verified on-device: with the AP's internet restored, the device recovered and displayed weather within the faster retry window.

## Follow-up 2 (boot-flow rework + connection diagnostics)

Reported: device "stuck" / "not connecting" after a power cycle. The mesh AP's Wi-Fi was reachable (device associated and got an IP), but outbound TLS/DNS to OpenWeather was intermittently failing (`start_ssl_client: -1`, `-29312 SSL EOF`, DNS failures).

Flow rework (in `application`, `network`, `weather_screen`, `weather`):
- **No automatic display diagnostic test at boot** — the RED/GREEN/BLUE/WHITE test now runs only via the `r` serial command. Boot goes straight to connecting.
- **Connection-status screen** (`renderConnecting(ssid, attempt, connected, ip)`): shows `Conectando a <SSID>...` + `Intento N` (yellow) while connecting, then `Conectado <IP>` + `Actualizando clima...` (blue) once Wi-Fi is up. Redrawn when the attempt count changes.
- **Boot Wi-Fi scan diagnostic** (`NetworkImpl::begin()`): prints all visible SSIDs and whether the target is visible — distinguishes "AP not in range" from "association/TLS failure".
- **TLS retry-once** in `OpenWeatherProvider`: transient HTTP/TLS failures are retried once immediately; 401/bad-status are not retried.
- `INetwork` gained `retryCount()` and `configuredSsid()` for the status screen.

Verified on-device: boot scan shows `HouseMesh` visible (-61 dBm), the device connects (IP assigned), and the connection-status screen tracks attempts. The remaining TLS failures were traced to the AP's unstable internet uplink (not the firmware); the device retries automatically and recovers when the uplink returns.

## Recommended next steps

- Sprint 7 candidates: NVS persistence of last-known weather, forecast/extra fields, or TLS certificate hardening.
- Optional: richer animation scenes (e.g. layered cloud parallax, day/night based on timestamp).

## Notes

- No new dependencies added.
- Language and API-language are compile-time config (change in `weather_credentials.h`; no firmware change needed to switch es/en).
- No hardware changes; display config (Sprint 1 baseline) untouched.
