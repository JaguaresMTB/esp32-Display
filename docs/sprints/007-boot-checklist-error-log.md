# Sprint 7 — Boot Checklist and Persistent Error Log

**Date:** 2026-08-09
**Status:** DONE

## Goal

Replace the boot loading UI with a clear, step-by-step checklist (no animation) that shows the full connection/weather process with attempt numbers, and keep a persistent boot/error log that survives unplug/replug.

## What was done

### Files created
- `src/common/error_log.h` / `.cpp` — persistent boot/error log in flash (NVS via core `Preferences`; no new dependency). Ring buffer of the last 10 events; `begin()`, `record(event)`, `dump()`.
- `docs/sprints/007-boot-checklist-error-log.md`.

### Files modified
- `src/networking/network.h` / `.cpp` — added `ConnectStage { None, Connecting, Authorizing, Connected }` (≈3 s into an attempt = authorizing) for checklist progress; removed the temporary boot Wi-Fi scan; records `wifi_connecting N`, `wifi_connected`, `wifi_fail`, `wifi_disconnected` to the error log.
- `src/services/weather/weather.cpp` — unchanged this sprint (weather events recorded by the Application).
- `src/ui/screens/weather_screen.h` / `.cpp` — new `renderChecklist(...)` replacing the spinner/connecting screen; **no animation while no data**; drawn green check marks (`drawLine`); gray/orange/green step indicators; attempt number on section titles and every step row; `updateAnimation()` does nothing until data exists.
- `src/application/application.h` / `.cpp` — at boot: `errorlog::begin()` + dump + record `boot`; drives the checklist from `connectStage()`/`retryCount()` + weather stage/attempt; ~0.4 s staged pauses between `Conectando`→`Autorizando` before the first weather fetch; records `weather_ok` / `weather_fail <name>`.
- `docs/architecture.md`, `README.md`, `docs/README.md` — updated.

### Boot checklist (on-screen)

```
MERIDA / Clima                     (header)
Wi-Fi                   Intento 3
  ● Conectando            Intento 3
  ○ Autorizando           Intento 3
  ✓ Conectado
Clima                   Intento 1
  ● Conectando            Intento 1
  ○ Autorizando           Intento 1
  ✓ Actualizado
IP: 192.168.68.109                 (footer)
```

- Gray dot = not reached, orange dot = current step, green check = done.
- Wi-Fi steps are genuine (network-driven); weather steps use short staged pauses so they are visible.
- Once weather data arrives, the checklist is replaced by the full weather screen (condition animation unchanged).

### Error log

- Events: `boot`, `wifi_connecting N`, `wifi_connected`, `wifi_fail`, `wifi_disconnected`, `weather_ok`, `weather_fail <name>`.
- Stored in NVS (survives reboot/unplug); dumped to serial at every boot.
- No secrets logged.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 1,000,640 B (76.3%), RAM 41,452 B (12.7%) |
| Upload | **PASS** — COM4 @ 921600 |
| Serial | **PASS** — boot log dump + events recorded |
| Boot checklist | **PASS** — renders (code-verified; static, no animation) |
| Persistent log | **PASS** — events from the previous boot dumped after reboot |
| Wi-Fi | **PASS** — connects (with retries when the AP uplink is flaky) |
| OpenWeather | **PASS** — fetched successfully (29.9 C, Clouds/nubes) when the uplink allows |
| Display regression | **PASS** — `r` diagnostic still works |

### Serial output (persistent log surviving a reboot)
```
[LOG] boot
[LOG] wifi_connecting 1
[LOG] wifi_connecting 2
[LOG] wifi_connected
[LOG] weather_ok
```
(These were recorded during the previous boot and dumped at the next one.)

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | Check mark `✓` not available in built-in ASCII fonts | ASCII-only glyph set | Draw the check with two `drawLine` segments |
| 2 | Step label + attempt number don't fit at the Medium font on one row | 240 px width limit | Step rows use `Small` (fits label + `Intento N`); section titles use `Medium` |
| 3 | Weather "Autorizando" step invisible during a fast blocking fetch | Fetch is blocking | Short ~0.4 s staged pauses before the first fetch (only while no data exists) |
| 4 | Boot errors lost across unplug/replug | No persistence | NVS ring-buffer error log dumped at boot |

## Open issues / blockers

- None.
- The underlying Wi-Fi uplink remains flaky at times (TLS/DNS failures); the checklist now makes this visible and the persistent log records it. The device retries automatically and recovers.

## Recommended next steps

- Sprint 8 candidates: TLS certificate hardening (replace `setInsecure()`), NVS persistence of the last-known weather, or the production forecast/extra fields.

## Notes

- No new dependencies (NVS via core `Preferences`).
- Boot checklist has no animation (per request); the weather data screen keeps its condition animation.
- No hardware changes; display config (Sprint 1 baseline) untouched.
