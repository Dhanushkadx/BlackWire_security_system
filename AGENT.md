# Project Memory

## Current Architecture

- `ALARM` no longer uses a borrowed `MY_SENS* pAny_sensor_array`.
- `ALARM` runtime state is now owned internally in `src/alarm.h` / `src/alarm.cpp` using:
  - `zone_open[TOTAL_DEVICES]`
  - `zone_available[TOTAL_DEVICES]`
  - `zone_enabled[TOTAL_DEVICES]`
  - `zone_alarm[TOTAL_DEVICES]`
  - `zone_last_alarm_ms[TOTAL_DEVICES]`
- `ZoneManager` is the source of persisted zone configuration.
- `zoneEngine` is the source of live zone state (`ZS_OPEN`, `ZS_CLOSE`, `ZS_FAULT`).
- `myAlarm_pannel.attachZoneManager(&gZoneManager)` is done during config load in `src/main.cpp`.

## Important Refactor Notes

- `src/ZoneManager.cpp` had entry/exit delay setters swapped and that was corrected:
  - `setExitDelay()` now writes `DS_EXIT_DELAY`
  - `setEntryDelay()` now writes `DS_ENTRY_DELAY`
- `ZONE_COUNT` was moved into `src/typex.h`.
- `src/event_bus.h` now includes `typex.h` instead of `statments.h` to avoid an include cycle.

## Command Parser

- Command strings for `universal_event_hadler()` are centralized in `src/universalEvent_commands.h`.
- The current command scheme is intentionally grouped and scalable:
  - `SYS=...`
  - `NET=...` / `NET?`
  - `AUTH=...`
  - `ZONE=...`
  - `ARM=...`
  - `PHONE...`
  - `CFG=...`
  - `OUT=...`
  - `POWER?`
  - `BUZZ=...`
  - `SIREN=...`
- `ZONE=03,EXIT,0` is the preferred zone parameter format.
- Zone rename now also lives under the same family:
  - `ZONE=03,NAME,Front Door`

## Build Status

- `pio run` succeeds after the above refactors.
- Flash usage is high, around `94%`, so future features should be added carefully.
