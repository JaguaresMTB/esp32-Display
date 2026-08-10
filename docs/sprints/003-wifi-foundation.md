# Sprint 3 — Wi-Fi Foundation

**Date:** 2026-08-09
**Status:** DONE

## Goal

Implement the network foundation for the future weather application: reliable, non-blocking Wi-Fi connectivity as an independent infrastructure layer, hidden behind a network abstraction. No weather API/HTTP/JSON in this sprint.

## What was done

### Files created
- `src/networking/network.h` — `networking::INetwork` interface, `networking::State` enum, `networking::getNetwork()` factory.
- `src/networking/network.cpp` — non-blocking connection state machine over Arduino `WiFi.h` (the only file that includes `<WiFi.h>`).
- `src/config/wifi_credentials.example.h` — committed credentials template with placeholders + instructions.
- `src/config/wifi_credentials.h` — **local, gitignored** file with real credentials (never committed).

### Files modified
- `src/main.cpp` — wired `networking::getNetwork()` into the application (composition root).
- `src/application/application.h/.cpp` — injected `networking::INetwork`; `begin()` starts a non-blocking connect; `update()` drives `network.update()`, logs state changes, and draws a small Wi-Fi status screen on the display (state + IP/RSSI).
- `.gitignore` — added `src/config/wifi_credentials.h`.
- `README.md`, `docs/README.md`, `docs/architecture.md` — updated.

### Network architecture

```
main.cpp -> Application -> networking::INetwork -> NetworkImpl -> <WiFi.h>
```

Application code never calls `WiFi.*`; only `networking/network.cpp` does. `main.cpp` wires the concrete implementation via the factory.

### Configuration / secrets mechanism

- Real credentials only in `src/config/wifi_credentials.h` (gitignored).
- `wifi_credentials.example.h` committed as the template.
- `network.cpp` includes credentials with an `__has_include` guard + empty-string fallback, so builds always succeed; empty credentials → connection skipped and logged.
- Password never printed.

### Connection / reconnection implementation

- State machine: `DISCONNECTED -> CONNECTING -> CONNECTED`, plus `RECONNECTING`; elapsed-time driven, no blocking loops.
- Connect timeout ~15 s; backoff 5 s → 10 s → 20 s → 40 s, capped at 60 s; failure counter resets on success.
- `WiFi.disconnect()` before each attempt for a clean station state.
- On connection success logs `[NET] connected` + `ssid=... ip=... rssi=... dBm`.
- Events logged as `[NET] initialization / connecting / connected / disconnected / reconnecting`.
- Display status screen: `Wi-Fi` header + `Connecting...` / `Connected` + IP + RSSI / `No network`.

## Results

| Check | Result |
|-------|--------|
| Build | **PASS** — Flash 788,782 B (60.2% of 1.3 MB app partition), RAM 38,884 B (11.9%) |
| Upload | **PASS** — COM4 @ 921600, USB-Serial/JTAG |
| Serial | **PASS** — startup + display test + `[NET]` events + heartbeat captured |
| Wi-Fi connection | **PASS** — connected to HouseMesh |
| IP | **PASS** — 192.168.68.114 |
| RSSI | **PASS** — approx. -60 dBm (range -69 to -59 across runs) |
| Display | **PASS** — color/text test + Wi-Fi status screen operational; no regression |

### TEST 1 — Normal connection (PASS)
```
[NET] initialization
[NET] CONNECTING (retry=0)
[NET] connected
[NET] ssid=HouseMesh ip=192.168.68.114 rssi=-69 dBm
```

### TEST 2 — Invalid credentials (PASS)
```
[NET] CONNECTING (retry=0)
[NET] connect failed (status=1)
[NET] DISCONNECTED in 5 s
[NET] CONNECTING (retry=1)
[NET] connect failed (status=1)
[NET] DISCONNECTED in 10 s
```
Heartbeat (`alive`) continued every 2 s throughout — no hang; application and display stayed responsive.

### TEST 3 — Recovery (PASS)
Disconnection detected instantly, bounded backoff, automatic rejoin once the AP was stable:
```
[NET] disconnected
[NET] RECONNECTING in 5 s
...
[NET] CONNECTING (retry=6)
[NET] connected
[NET] ssid=HouseMesh ip=192.168.68.114 rssi=-59 dBm
```

## Problems found and solutions

| # | Problem | Root cause | Solution |
|---|---------|-----------|----------|
| 1 | After the AP power cycle, reconnection kept failing with `status=4` (WL_CONNECT_FAILED) even with correct credentials | Known ESP32 behavior: repeated `WiFi.begin()` after a disconnect keeps failing without a clean station state; also the mesh AP needs ~1 min to stabilize after a reboot | Added `WiFi.disconnect()` before each `WiFi.begin()` attempt |
| 2 | `RECONNECTING` state polled `WiFi.status()` without re-invoking `WiFi.begin()` | State-machine bug: the reconnect state ran `handleAttempt()` immediately instead of waiting for the scheduled attempt time | Restructured update() so `Disconnected`/`Reconnecting` wait for `_nextAttemptAt`, then call `startConnect()` (which invokes `WiFi.begin`) |
| 3 | First background serial capture for TEST 3 captured nothing | `Start-Job` background jobs are killed when the launching PowerShell session ends | Switched to a detached `Start-Process` capture script writing to a temp log file |
| 4 | Transient failures during TEST 3 appeared as "still broken" | The mesh AP takes time to become fully reachable after power-on (confirmed via a temporary scan showing `HouseMesh` eventually visible) | Diagnosed with a temporary `WiFi.scanNetworks()` logging block; removed after verification |
| 5 | Empty credentials at first boot caused a connection attempt with an empty SSID | Build without a local credentials file | `begin()` skips connection and logs `no credentials configured` when `WIFI_SSID` is empty |

## Open issues / blockers

- None. Wi-Fi connectivity works (connect, invalid-cred failure handling, auto-reconnect).
- Note: the mesh AP takes roughly 1 minute to become reachable after a power cycle; the firmware handles this with bounded retries and no blocking.

## Recommended next steps

- Sprint 4 candidate: HTTP client and a weather service layer (`src/networking/http`, `src/services/weather`) that use the already-connected network through `networking::INetwork`. JSON parsing and API-key handling with the same gitignored-config approach.
- Optionally reduce the `alive` heartbeat noise or gate it behind a debug flag when adding the weather UI.

## Notes

- `src/config/wifi_credentials.h` is gitignored; the real password was never committed or logged.
- The Wi-Fi status screen (`Wi-Fi` header, `Connecting...` / `Connected` + IP + RSSI) provides quick visual feedback for hardware bring-up.
- No hardware changes; display config (Sprint 1 baseline) untouched.
