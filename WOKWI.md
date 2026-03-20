# Wokwi Quick Start

## Files

- `wokwi.toml`
- `diagram.json`

## Start The Simulator

1. Build the project first:
   - `pio run`
2. In VS Code, run:
   - `Wokwi: Start Simulator`
3. Keep the Wokwi simulator tab visible.

## See Serial Output

This project uses RFC2217 serial forwarding.

- Port: `4000`
- Baud: `115200`

In VS Code Serial Monitor:

1. Open Serial Monitor
2. Select TCP mode
3. Connect to:
   - `localhost:4000`
4. Set baud rate to:
   - `115200`

## Wokwi Wiring

### Inputs

Slide switches are used as latched zone inputs:

- `zone0` -> `GPIO35`
- `zone1` -> `GPIO34`
- `zone2` -> `GPIO36`
- `zone3` -> `GPIO39`

These are the 4 GPIO zones configured in `src/main.cpp`.

### Outputs

LEDs are connected to:

- `GPIO19` -> alarm relay LED
- `GPIO12` -> arm LED
- `GPIO13` -> disarm LED
- `GPIO2`  -> buzzer LED
- `GPIO27` -> RF/status LED

## UART Command Tests

Use these in the serial monitor:

### System

- `SYS=STATUS`
- `SYS=LOG`
- `SYS=REBOOT`

### Network

- `NET?`
- `NET=AP,1`

### Auth

- `AUTH=1234`

### Arm / Disarm

- `ARM=HOME`
- `ARM=DISARM`
- `ARM=PANIC`
- `ARM=ALARM`

### Zone Config

- `ZONE=03,EXIT,0`
- `ZONE=03,ENTRY,1`
- `ZONE=03,BYPASS,1`
- `ZONE=03,NAME,Front Door`

### Phone Config

- `PHONE=01,+94712345678`
- `PHONE.SMS=01,1`
- `PHONE.CALL=01,1`

### Timing

- `CFG=ENTRY_DELAY,10`
- `CFG=EXIT_DELAY,15`

### Outputs

- `OUT=1,1`
- `OUT=1,0`
- `OUT=2,1`
- `OUT=2,0`

### Misc

- `POWER?`
- `BUZZ=CHIME`
- `SIREN=1`
- `SIREN=0`

## Notes

- GSM is not modeled in this Wokwi setup.
- This setup is intended only for:
  - UART command parsing
  - zone input changes
  - visible output pin changes
- If serial output does not appear, make sure the simulator tab is still open and connected.
