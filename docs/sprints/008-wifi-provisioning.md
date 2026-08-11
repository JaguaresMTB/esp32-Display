# Sprint 8 — Wi-Fi Provisioning & Reconfiguration

**Date:** 2026-08-09
**Status:** DONE

## Goal

Allow configuring (and reconfiguring) Wi-Fi without a computer or reflashing: first-time provisioning via SoftAP + captive portal, and explicit reconfiguration via the onboard BOOT button. Credentials persist in NVS; failed reconfiguration must never destroy a valid active configuration.

## Hardware button

- **BOOT button** = GPIO9 (ESP32-C3 boot/strapping pin; the Super Mini BOOT button connects it to GND, active-low). Used as the explicit provisioning trigger via a ~3 s long-press.
- **RST button** = hardware reset, never used as an application input.
- The firmware only reads GPIO9 as `INPUT_PULLUP` after startup; it never drives it, so normal boot and USB flashing are unaffected (verified: uploads and normal boots work throughout testing).

## Architecture

```
Application
   ▼
INetwork (extended)
   ▼
NetworkImpl ── normal Wi-Fi state machine (reads credentials from NVS)
   └── ProvisioningManager
         ├── SoftAP "WeatherDisplay-XXXX" (open, XXXX = last 4 hex of MAC)
         ├── DNSServer (captive portal → 192.168.4.1)
         ├── WebServer (HTTP configuration portal)
         └── WifiCredentialStore (NVS namespace "wifi", separate from "bootlog")
```

- New files: `src/networking/wifi_credentials.{h,cpp}`, `src/networking/provisioning.{h,cpp}`.
- Modified: `src/networking/network.{h,cpp}` (provisioning integration, NVS credentials at boot), `src/config/pins.h` (`BOOT_BUTTON_PIN 9`), `src/application/application.{h,cpp}` (BOOT long-press, provisioning UI, `p` command), `src/ui/screens/weather_screen.{h,cpp}` (`renderProvisioning`).
- Dependencies: **none added** — `WebServer` and `DNSServer` are Arduino-ESP32 core libraries; NVS via core `Preferences`.

## Provisioning flow / state machine

- `IDLE` → `PROVISIONING` (SoftAP + DNS + HTTP up) → `CONFIGURING` (awaiting submit) → `TESTING_CONNECTION` (candidate connect, bounded ~15 s, AP stays up via AP_STA) → success → `COMMIT` (save to NVS, stop AP/DNS/HTTP) → `NORMAL`. Failure → back to `CONFIGURING`, AP stays up, user can retry.
- **Candidate vs active:** submitted credentials are tested first; committed to NVS **only after a successful connection**. A failed reconfiguration leaves the stored active config untouched.
- **Entry conditions:** (1) no stored credentials at boot → automatic provisioning; (2) BOOT long-press (~3 s, non-blocking) at any time → provisioning. A temporary Wi-Fi outage does **not** enter provisioning or erase credentials.

## User workflow

- **First use:** power on (no credentials) → provisioning starts → connect to `WeatherDisplay-XXXX` → open `http://192.168.4.1` → select/type SSID + password → Save & Connect → device validates, commits, connects, weather appears.
- **Reconfiguration:** hold BOOT ~3 s → provisioning → connect to the new AP → configure → device validates → new network becomes active only on success.
- **Failed reconfiguration:** portal shows "Connection failed. Please verify the Wi-Fi name and password."; the old active config remains; retry possible.
- **Recovery:** if credentials are wrong, stay in the portal and re-enter correct ones; reboot loops do not occur.

## Portal

- `GET /` → lightweight HTML (device name, IP, cached scan list with SSID/RSSI/encryption, manual SSID field, password, Save & Connect, status area that polls `/status`).
- `POST /configure` → candidate → testing.
- `GET /status` → JSON `{state, result}`.
- Unknown routes → 302 to `http://192.168.4.1/`. Manual URL always works.
- Scan cached ~30 s (not per request); manual SSID entry always available (hidden networks, absent scan).

## Tests

| Test | Result |
|------|--------|
| BOOT button: GPIO9 reads, long-press triggers, short press ignored | **PASS** — `BOOT pressed (pin=9)` → `alive b=0` → `BOOT long press -> provisioning` |
| First boot (no credentials) → provisioning | **PASS** — `no stored credentials; entering provisioning mode` → `softap=WeatherDisplay-2BA0` |
| Portal + scan accessible (`http://192.168.4.1`) | **PASS** — user connected from phone, saw networks |
| First-time successful provisioning | **PASS** — `provisioning_completed`, connected, weather works |
| Invalid password (failed provisioning) | **PASS** — `provisioning_connection_failed`, portal remains available |
| Successful reconfiguration (BOOT long-press) | **PASS** — `provisioning_completed`, new config active |
| **Failed reconfiguration preserves active config** | **PASS** — after `provisioning_connection_failed`, HouseMesh remained active and weather worked |
| Reboot persistence | **PASS** — credentials load from NVS; provisioning does NOT restart; connects + weather |
| Temporary Wi-Fi failure | **PASS** — existing retry/reconnect; no auto-provisioning, credentials intact (Sprint 3 behavior preserved) |
| Explicit re-provisioning after failure | **PASS** — BOOT long-press re-entered provisioning and reconfigured |
| Boot/flash regression | **PASS** — USB flashing and normal boot work throughout |
| Display regression | **PASS** — weather screen, checklist, provisioning screen, `r` diagnostic |
| Error-log regression | **PASS** — `provisioning_started/connection_success/connection_failed/completed` recorded; no secrets |

## Problems and solutions

| # | Problem | Solution |
|---|---------|----------|
| 1 | Credentials were compile-time only; no runtime provisioning | Added `WifiCredentialStore` (NVS) + `ProvisioningManager`; NVS became the sole runtime source |
| 2 | Captive-portal auto-detection is OS-dependent | Always reachable manual URL `http://192.168.4.1`; DNS captures portal requests |
| 3 | Failed reconfiguration could destroy active config | Candidate-then-commit model: commit to NVS only after a successful connection |
| 4 | BOOT long-press must not block the loop | Elapsed-time long-press detection in `Application::update()` |
| 5 | Debugging required knowing the button state | Temporary diagnostics (`BOOT pressed`, `alive b=`) used during testing, then cleaned up |

## Security considerations

- Provisioning AP is **open** (no password). Trade-off documented: it is only active during provisioning (first boot or explicit BOOT trigger) and requires physical proximity; it is fully stopped in normal operation.
- Portal is HTTP on the private provisioning AP (no HTTPS) — acceptable because it is isolated from the internet.
- Wi-Fi passwords are never logged, never written to the error log, never exposed by the portal, never committed, never included in docs.

## Open issues

- None blocking. Note: on the development bench, releasing the USB-CDC serial port can reset the device (USB-Serial/JTAG behavior) — this does not occur when the device runs standalone; it can interrupt an in-progress provisioning session during development.

## Recommended next steps

- Sprint 9 candidates: TLS certificate hardening (replace `setInsecure()`), NVS persistence of last-known weather, forecast/extra fields, or a physical setup button using the existing `enterProvisioningMode()` hook.

## Notes

- No new libraries (WebServer/DNSServer/Preferences are core).
- Provisioning is an additional capability; the Sprint 3 normal Wi-Fi state machine (retry/reconnect, non-blocking) is preserved.
- No hardware changes; display config (Sprint 1 baseline) untouched.
