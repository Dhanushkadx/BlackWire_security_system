# Project Memory — BlackWire Security System

Last updated: 2026-03-26 | Branch: mqtt_improve

---

## Project Overview

ESP32-based IoT alarm security system. Dual-core 240MHz, 4MB PSRAM, SPIFFS filesystem.
Up to 48 zones: GPIO wired, ADS1115 analog, 433MHz RF wireless.
Communication: WiFi + MQTT, GSM (SIM800), Async Web portal (REST + WebSocket), OTA updates.
Outputs: siren relay, buzzer, NeoPixel LED strip.

---

## Zone Pipeline (Data Flow)

```
GPIO / ADS1115 / RF
      ↓ RawInputEvent (event_bus queue)
  ZoneEngine  (debounce, momentary, stable state)
      ↓ ZoneEvent (event_bus queue)
  Task1 → ALARM state machine
      ↓ ZoneBroadcast (qZoneBroadcast queue)
  TaskBroadcastRouter → MQTT / WebSocket / GSM
```

---

## Zone ID Map

| ID Range | Source            |
|----------|-------------------|
| 0–7      | GPIO inputs       |
| 8–15     | ADS1115 Card A (0x48) |
| 16–23    | ADS1115 Card B (0x49) |
| 32–47    | RF / reserved     |

---

## Key Classes and Responsibilities

| Class/File              | Responsibility |
|-------------------------|----------------|
| `ALARM` (alarm.h/cpp)   | State machine + zone policy engine (arm/disarm, entry/exit delay, alarm trigger) |
| `ZoneEngine` (zone_engine.h/cpp) | Debouncing, momentary handling, per-zone runtime state (ZS_OPEN/CLOSE/FAULT) |
| `ZoneManager` (ZoneManager.h/cpp) | Persistent zone metadata: names, type flags, state flags → SPIFFS `zones.bin` |
| `ZoneStorage` (ZoneStorage.h/cpp) | Binary file read/write with CRC16-CCITT + atomic rename |
| `RemoteStorage` (RemoteStorage.h/cpp) | 8-slot RF remote storage (`remotes.bin`) |
| `MqttBroker` (mqtt_brokerx.h/cpp) | MQTT pub/sub, TX queue, attribute sync, RPC |
| `GsmBroker` (gsm_broker.h/cpp) | SIM800 SMS + voice call management |
| `AsyncWebServer` (async_web_server.h/cpp) | REST endpoints + static SPIFFS file serving |
| `socket_function` (socket_function.h/cpp) | WebSocket message handlers |
| `universalEventHandler` (universalEventx.h/cpp) | Unified command parser for serial/MQTT/SMS/web |
| `EventBus` (event_bus.h/cpp) | FreeRTOS queue-based event distribution |
| `msg_store` (msg_store.h/cpp) | Offline MQTT message queue for failed publishes |

---

## ALARM Class — Internal State (Refactored)

ALARM no longer uses a borrowed `MY_SENS* pAny_sensor_array`.
All runtime zone state is owned internally:

```cpp
bool zone_open[TOTAL_DEVICES]
bool zone_available[TOTAL_DEVICES]
bool zone_enabled[TOTAL_DEVICES]
bool zone_alarm[TOTAL_DEVICES]
uint32_t zone_last_alarm_ms[TOTAL_DEVICES]
```

`myAlarm_pannel.attachZoneManager(&gZoneManager)` is called during config load in `main.cpp`.
`ZoneManager` → persisted metadata only.
`ZoneEngine` → live debounced state only.

### Alarm State Machine (eMain_state)

```
DEACTIVE ←→ SYS1_IDEAL / SYS2_IDEAL ←→ ENTRY_TIMER_START
                                              ↓
EXIT_TIMER_START → SYS1_IDEAL          ALARM_CALLING / PANIC
```

---

## FreeRTOS Task Architecture

| Task        | Core | Period  | Responsibility |
|-------------|------|---------|----------------|
| Task1       | 1    | 10ms    | Alarm watcher, zone event consumer, serial CLI |
| Task2       | 1    | —       | GSM SMS handler (suspended by default) |
| Task4       | 1    | —       | GSM modem manager |
| Task5       | 1    | 500ms   | Voice call state machine |
| Task7       | 1    | 10ms    | GSM status LED |
| Task8       | 0    | 50ms    | MQTT communication loop + TX queue drain |
| Task9       | 1    | —       | IO: buzzer, siren, pixel control |
| Task10      | 1    | 10ms    | AC power detection, battery monitoring |
| pollTask    | 1    | —       | GPIO + ADS1115 polling → RawInputEvent |
| zoneEngineTask | 1 | —       | Consumes RawInputEvent → ZoneEngine → ZoneEvent |
| zoneTickTask | 1   | —       | Debounce/momentary timer ticks |
| TaskBroadcastRouter | 1 | — | Routes ZoneBroadcast → MQTT/WS/GSM |
| RF433 task  | 1    | —       | RCSwitch polling for RF codes |
| WS TX task  | 0    | —       | WebSocket TX queue drainer |

---

## MQTT Architecture (Refactored)

- **No separate MQTT TX task.**
- TX queue drained inside `mqtt_com_loop()` (Task8, every 50ms / drain every 30s).
- All other tasks enqueue via `mqtt_tx_enqueue(topic, payload, retain)`.
- **Only Task8 touches `PubSubClient` / `WiFiClientSecure`.**
- Fixes SSL internal errors caused by concurrent access.

### Topics Published

```
blackwire/{deviceId}/status
blackwire/{deviceId}/state
blackwire/{deviceId}/telemetry
blackwire/{deviceId}/zone/{z}/state
blackwire/{deviceId}/alarm/event
blackwire/{deviceId}/events/arm
blackwire/{deviceId}/events/power
blackwire/{deviceId}/attributes
```

### Topics Subscribed

```
blackwire/{deviceId}/attr/res    ← attribute fetch responses (cfgIndex state machine)
blackwire/{deviceId}/attr/set    ← unsolicited server-push (only cfgIndex key acted upon)
blackwire/{deviceId}/rpc/req     ← RPC commands from server
```

### cfgIndex-Driven Attribute Sync

Version-gated sync on every MQTT connect. Only keys whose server `ver` exceeds local `ver` are fetched.

**Boot flow:**
1. `configLoad(0)` → `mqtt_load_cfg_index()` loads `/cfgIndex.json` into `g_localCfgIdx[]`
2. MQTT connect → state machine starts with `AFS_FETCH_META`
3. Requests `cfgIndex` shared attr → compares versions → builds `g_pendingFetchMask`
4. Fetches each pending key one at a time
5. After each key applied: saves `/cfgIndex.json` + publishes `cfgIndex` client attr immediately

**State machine (`eAttrFetchState`):**
```
AFS_IDLE      → timer elapsed → AFS_FETCH_META
AFS_FETCH_META → sends attr/request key="cfgIndex" → AFS_WAIT_META
AFS_WAIT_META  → response → process_server_cfg_index() → AFS_FETCH_KEY
               → timeout/missing → publish {"cfgIndex":"Invalid"} → AFS_IDLE (retry 30s)
AFS_FETCH_KEY  → lowest pending bit → sends attr/request → AFS_WAIT_KEY
AFS_WAIT_KEY   → response → apply_data_key_content() → clear bit → AFS_FETCH_KEY
               → timeout → clear bit → AFS_FETCH_KEY
```

**`attr/set` force push** (server sends unsolicited):
```json
{"device":"blackwire_XXXX", "data": {"cfgIndex": {"config":{"ver":N,"updatedTs":N}, ...}}}
```
Only `cfgIndex` key inside `data` is acted upon — direct key pushes (config, users, zones) are ignored.

**`attr/res` response** (no key wrapper — device knows key from `g_attrFetchKeyIdx`):
```json
{"id":N, "device":"blackwire_XXXX", "value": <data>}
```

**cfgIndex key map** (8 keys, stored in `/cfgIndex.json`):

| Key | verField | tsField |
|-----|----------|---------|
| `config` | `cfg_config_ver` | `cfg_config_ts` |
| `users` | `cfg_users_ver` | `cfg_users_ts` |
| `z_atr00_07` … `z_atr40_47` | `cfg_z_atr00_07_ver` … | `cfg_z_atr00_07_ts` … |

**Attribute key formats:**

| Key | Value format | Notes |
|-----|-------------|-------|
| `config` | `{"ver":N, "updatedTs":N, "enDelay":N, ...}` | No version guard — cfgIndex is the gatekeeper |
| `users` | `{u0:{en,nm,tp,smsEn,callEn}, u1:{...}, ...}` | Named-key object, 8 slots; saved as flat array to `/users.json` using same short keys |
| `z_atr00_07` | `{"z0":{...}, "z7":{...}}` | Zones 0–7, display Z01–Z08 |
| `z_atr08_15` | `{"z8":{...}, "z15":{...}}` | Zones 8–15, display Z09–Z16 |
| `z_atr16_23` | `{"z16":{...}, "z23":{...}}` | Zones 16–23, display Z17–Z24 |
| `z_atr24_31` | `{"z24":{...}, "z31":{...}}` | Zones 24–31, display Z25–Z32 |
| `z_atr32_39` | `{"z32":{...}, "z39":{...}}` | Zones 32–39, display Z33–Z40 |
| `z_atr40_47` | `{"z40":{...}, "z47":{...}}` | Zones 40–47, display Z41–Z48 |

Zone keys are **0-based** in JSON (z0–z47), matching internal array index directly. Display layer adds +1 (Z01–Z48).

**Zone object fields:** `n` (name), `by` (bypass), `ed` (entry delay), `xd` (exit delay), `x24` (24h), `pm` (perimeter), `sl` (silent), `rf` (RF sensor), `ch` (chime)

**Client attr published after each key applied (`attr/pub`):**
```json
{"cfgIndex": {"config":{"ver":N,"updatedTs":N}, "users":{"ver":N,"updatedTs":N}, ...}}
```
Same nested format as server cfgIndex. Stored on SPIFFS as `/cfgIndex.json` (same structure, no outer `cfgIndex` wrapper).

---

## Command Parser — universalEvent_commands.h

All command strings for `universal_event_hadler()` are centralized here.
Format examples:

| Family    | Example |
|-----------|---------|
| `SYS=`    | `SYS=RESET` |
| `NET=`    | `NET=SSID,pass` / `NET?` |
| `AUTH=`   | `AUTH=user,pass` |
| `ZONE=`   | `ZONE=03,EXIT,0` / `ZONE=03,NAME,Front Door` |
| `ARM=`    | `ARM=1` / `ARM=0` |
| `PHONE`   | `PHONE=+94xxxxxxxxx` |
| `CFG=`    | `CFG=entry,30` |
| `OUT=`    | `OUT=relay,1` |
| `POWER?`  | `POWER?` |
| `BUZZ=`   | `BUZZ=1` |
| `SIREN=`  | `SIREN=1` |

---

## Storage Files (SPIFFS)

| File                    | Format         | Managed by      | Notes |
|-------------------------|----------------|-----------------|-------|
| `zones.bin`             | Binary + CRC16 | ZoneStorage     | MY_SENS[48] + names[48][16] |
| `remotes.bin`           | Binary + CRC16 | RemoteStorage   | 8 RF remote slots |
| `config.json`           | JSON (flat)    | ConfigManager   | WiFi, MQTT, GSM, alarm timers — no wrapper, keys at root |
| `users.json`            | JSON array     | mqtt_brokerx, call_backs, socket_function | 8 user slots: `[{en,nm,tp,smsEn,callEn},…]` short keys only |
| `cfgIndex.json`         | JSON           | mqtt_brokerx    | Nested: `{"config":{"ver":N,"updatedTs":N}, "users":{…}, …}` |
| `file_index.txt`        | plain text     | msg_store       | Offline MQTT queue file index pointer |

All binary files use atomic write (temp file + rename) for power-loss safety.
`cfgIndex.json` is also written atomically (tmp file + rename).
Auto-init with defaults if CRC fails.

### Web Portal
- **`index.html`** — only active portal page (all other .html files are abandoned)
- `contact.html`, `info.html`, `zone.html` — **abandoned, safe to delete from /data**

---

## Zone Flags Reference

### MY_SENS.device_state bits
```
DS_EXIT_DELAY, DS_ENTRY_DELAY, DS_ENABLE, DS_BYPASSED,
DS_ALARM, DS_LAST_STATE, DS_AVAILABLE
```

### MY_SENS.device_type bits
```
DT_24H, DT_RF, DT_SILENT, DT_PERIMETER
```

### ZoneConfig (runtime, ZoneEngine)
```cpp
bool bypass
bool is24h
bool chime
uint16_t debounce_ms       // default 50ms
uint16_t momentary_hold_ms // 0 = not momentary
```

---

## RF Remote Control (433MHz)

- Library: RC-Switch on GPIO 4
- Protocol: EV1527 (4-bit or 8-bit command suffix)
- 8 slots in RemoteStorage
- Learn mode: `rfScanArm(SCAN_REMOTES, slot, 15s)` triggered from web portal

---

## Important Refactor Notes (Historical)

- `ZoneManager.cpp`: entry/exit delay setters were swapped → fixed:
  - `setExitDelay()` writes `DS_EXIT_DELAY`
  - `setEntryDelay()` writes `DS_ENTRY_DELAY`
- `ZONE_COUNT` moved to `typex.h` (compile-time constant, `TOTAL_DEVICES = 48`)
- `event_bus.h` includes `typex.h` instead of `statments.h` to avoid include cycles

---

## RAM Optimisations Applied (2026-03-22)

| Change | Saving |
|--------|--------|
| Deleted unused global `StaticJsonDocument<6144> docz` in `async_web_server.cpp` | ~6 KB permanent |
| `eventBusInit(96,96)` → `(32,32)` in `main.cpp` | ~896 B |
| `MQTT_TX_PAYLOAD_MAX` 512 → 384 in `mqtt_brokerx.h` | ~3.1 KB queue RAM |
| `MAX_MSG_SIZE` 512 → 384 in `msg_store.h` | ~1.3 KB queue RAM |
| Replaced all Arduino `String` in `socket_function.cpp` with `char[]` (9 sites) | heap fragmentation |
| Replaced `String` in `universalEventx.cpp` (ipAddress, macAddress, wifiStatus) | heap fragmentation |
| Replaced `WiFi.localIP().toString()` String in `mqtt_brokerx.cpp` | heap fragmentation |

Task stack sizes NOT yet reduced — measure with `uxTaskGetHighWaterMark()` first.

## config.json Format

- Flat JSON at root — no `config` or `sysconf` wrapper
- `get_config_root()` in `config_manager.cpp` simply returns `doc.as<JsonObject>()`
- `configSave()` writes all fields flat using the same key names as config.json

Key names (new, used everywhere):

| Field | Key | Type |
|-------|-----|------|
| Entry delay | `enDelay` | uint8 |
| Exit delay | `xtDelay` | uint8 |
| Bell timeout | `bellTout` | uint16 |
| Beep timeout | `beepTout` | uint16 |
| Sensor debounce | `debTm` | uint8 |
| Siren enable | `bellEn` | bool |
| Beep enable | `beepEn` | bool |
| Call enable | `callEn` | bool |
| Call attempts | `callAtmpt` | uint8 |
| CLI access level | `cliLevel` | uint8 |
| Last SMS sender | `lastSender` | string |
| WiFi STA enable | `wstaEn` | bool |
| WiFi AP enable | `wapEn` | bool |
| WiFi SSID | `wssid` | string |
| AP SSID | `wapssid` | string |
| WiFi password | `wstaPw` | string |
| MQTT enable | `mqttEn` | bool |
| MQTT server | `mqttServer` | string |
| MQTT port | `mqttPort` | uint32 |
| MQTT username | `mqttUser` | string |
| MQTT password | `mqttPass` | string |

## Build Status

- `pio run` succeeds on mqtt_improve branch (2026-03-25)
- Flash usage: ~**95.4%** (1,250,725 / 1,310,720 bytes) — near limit
- RAM usage: ~**17.0%** (55,664 / 327,680 bytes) — plenty of headroom
- OTA: functional (AsyncElegantOTA)
- SSL errors: resolved by single-task MQTT access
- `MQTT_SECURE` is defined in `mqtt_brokerx.h` (TLS enabled) — removing saves ~100–130 KB flash if broker doesn't require SSL

### Task Stack Sizes
| Task | Stack (words) | Notes |
|------|--------------|-------|
| Task3 (LCD) | 3072 | |
| Task7 (GSM LED) | 3072 | |
| Task8 (MQTT) | 8192  | Reduced from 10240 — cfgIndex publish uses snprintf (no stack-heavy StaticJsonDocument) |
| Task10 (power) | 3524 | |

### Flash Usage Notes
- ESP32 SDK/WiFi/SSL prebuilt blobs: ~68% of flash (untouchable)
- Your source code: ~15.6% (194 KB) — reducible
- 523 unguarded `Serial.print*` calls — guarding with `#ifdef _DEBUG` saves ~19 KB
- `_DEBUG` is currently hardcoded `#define _DEBUG` in `mqtt_brokerx.h` — move to build flag to enable stripping

---

## Hardware Pin Summary

| Function        | Pin(s) |
|-----------------|--------|
| GPIO inputs     | 35, 34, 36, 39 |
| Relay/Outputs   | 13, 27, 2, 18, 19 |
| RF 433MHz RX    | GPIO 4 |
| GSM Serial1     | TX=16, RX=33, RST=22 |
| AC detect       | GPIO 21 |
| I2C (ADS/LCD)   | Wire (default SDA/SCL) |
| ADS1115 addrs   | 0x48, 0x49, 0x4A, 0x4B |
| LCD             | 0x3C |

---

## PlatformIO Build & Debug

### Environment
- Framework: Arduino / ESP-IDF
- Tool: PlatformIO (`pio`)
- Baud: 115200
- Config: `platformio.ini`
- Shell used in this workspace: PowerShell

### Commands

#### Compile only
```powershell
pio run
```

#### Compile + Upload
```powershell
pio run --target upload
```

#### Read serial (15 sec capture)
```powershell
Start-Sleep -Seconds 2
$end = (Get-Date).AddSeconds(15)
$p = Start-Process -FilePath pio -ArgumentList 'device','monitor','--baud','115200' -PassThru -NoNewWindow
try {
  while ((Get-Date) -lt $end -and -not $p.HasExited) {
    Start-Sleep -Milliseconds 200
  }
} finally {
  if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
  }
}
```

#### Full loop (compile -> upload -> read serial)
```powershell
pio run --target upload
Start-Sleep -Seconds 3
$end = (Get-Date).AddSeconds(15)
$p = Start-Process -FilePath pio -ArgumentList 'device','monitor','--baud','115200' -PassThru -NoNewWindow
try {
  while ((Get-Date) -lt $end -and -not $p.HasExited) {
    Start-Sleep -Milliseconds 200
  }
} finally {
  if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
  }
}
```

### Workflow Rules
- Always compile first and fix all compile errors before uploading.
- After upload, wait 3 seconds for ESP32 boot, then read serial.
- Capture serial for at least 15 seconds.
- Look for tags: `[ZONE]`, `[MQTT]`, `[RF]`, `[OTA]`, `[ALARM]`, `[ERROR]`
- Boot complete marker: `=== BOOT COMPLETE ===`
- When explicitly doing a debug loop, do not stop for confirmation between compile, upload, and serial capture.
- If upload fails, check port with:

```powershell
pio device list
```
