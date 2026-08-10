# Documentation

## Index

- [Pinout / wiring reference](pinout.md) — single source of truth for hardware connections.
- [Architecture](architecture.md) — project structure, dependency direction, display abstraction.
- [Sprint 1 — Hardware validation](sprints/001-hardware-validation.md) — completed sprint.
- [Sprint 2 — Firmware Architecture Foundation](sprints/002-firmware-architecture-foundation.md) — completed sprint.
- [Sprint 3 — Wi-Fi Foundation](sprints/003-wifi-foundation.md) — completed sprint.
- [Sprint 4 — OpenWeather Integration](sprints/004-openweather-integration.md) — completed sprint.
- [Sprint 5 — Weather Display UI and Periodic Refresh](sprints/005-weather-display-ui.md) — completed sprint.
- [Sprint 6 — Weather Animation and Spanish UI](sprints/006-weather-animation-i18n.md) — completed sprint.
- [Sprint 7 — Boot Checklist and Persistent Error Log](sprints/007-boot-checklist-error-log.md) — completed sprint.
- [Sprint template](sprints/000-TEMPLATE.md) — template for future sprint documents.

## Convention: sprint documentation

A **sprint** is a unit of work (feature, hardware validation, refactor, fix) that is documented **when it is considered done** — i.e. when the work has reached a stopping point that we want a record of.

Rules:

1. Every completed sprint gets one file under `docs/sprints/`, named `<NNN>-<slug>.md` (e.g. `002-display-render-fix.md`).
2. Use the [sprint template](sprints/000-TEMPLATE.md) as the starting point.
3. A sprint document must record:
   - Goal / acceptance criteria
   - What was done (files, configs, commands)
   - Results (build/upload/serial/observations)
   - **Problems found and how they were solved** (table)
   - Open issues / blockers (even if the sprint is "closed", record what remains)
   - Recommended next steps
4. If a sprint leaves a problem unsolved, it is moved to the *Open issues* of that sprint and picked up in the next one. Do not delete history.
5. Update the README status table when a sprint changes overall project status.

## Template usage

Copy `sprints/000-TEMPLATE.md` to `sprints/<NNN>-<slug>.md`, fill in the sections, then add a link in this index.
