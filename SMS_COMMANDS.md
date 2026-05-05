# BlackWire SMS Commands

All commands are **case-insensitive**. Send to the device SIM number.

---

## Arm / Disarm

| SMS | Action | Reply |
|---|---|---|
| `HOME` | Home arm (perimeter sensors only) | `Home Armed OK` |
| `AWAY` | Away arm (all sensors) | `Away Armed OK` |
| `DISARM` | Disarm the system | `Disarmed OK` |
| `ARM=PANIC` | Trigger panic alarm | — |

---

## Zone Simulation (Testing Only)

Simulates a zone state without needing physical sensors.

| SMS | Action | Reply |
|---|---|---|
| `ZONE=nn,0` | Force zone nn → **Closed** (normal) | `OK` |
| `ZONE=nn,1` | Force zone nn → **Open** (triggered) | `OK` |
| `ZONE=nn,2` | Force zone nn → **Fault** | `OK` |

Zone numbers are **0-based** (zone 0 = Z01 in the app, zone 1 = Z02, etc.).

**Examples:**
```
ZONE=00,1    → open zone Z01
ZONE=05,0    → close zone Z06
ZONE=47,1    → open zone Z48
```

---

## Siren & Buzzer

| SMS | Action |
|---|---|
| `SIREN=1` | Turn on siren relay + buzzer |
| `SIREN=0` | Turn off siren relay + buzzer |
| `BUZZ=CHIME` | Single chime beep |

---

## System Info

| SMS | Action | Reply |
|---|---|---|
| `SYS=STATUS` | Full status (WiFi, IP, GSM, time, arm mode) | Full status SMS |
| `NET?` | GSM carrier name and signal strength | `Carrier:xxx\nGSM RSSI:xx` |
| `POWER?` | AC power and battery status | Power status SMS |

---

## Configuration

| SMS | Action | Reply |
|---|---|---|
| `CFG=ENTRY_DELAY,30` | Set entry delay to 30 seconds | `OK` |
| `CFG=EXIT_DELAY,15` | Set exit delay to 15 seconds | `OK` |
| `ZONE=nn,NAME,Front Door` | Rename zone nn | `OK` |

---

## Phone Contacts

| SMS | Action |
|---|---|
| `PHONE=1,+94712345678` | Set contact slot 1 phone number |
| `PHONE.SMS=1,1` | Enable SMS alerts for contact 1 |
| `PHONE.SMS=1,0` | Disable SMS alerts for contact 1 |
| `PHONE.CALL=1,1` | Enable call alerts for contact 1 |
| `PHONE.CALL=1,0` | Disable call alerts for contact 1 |

---

## Output Relays

| SMS | Action | Reply |
|---|---|---|
| `OUT=1,1` | Relay 1 ON | `Relay 1 on OK` |
| `OUT=1,0` | Relay 1 OFF | `Relay 1 off OK` |
| `OUT=2,1` | Relay 2 ON | `Relay 2 on OK` |
| `OUT=2,0` | Relay 2 OFF | `Relay 2 off OK` |

---

## System

| SMS | Action |
|---|---|
| `SYS=REBOOT` | Reboot device (5 sec delay) |
| `NET=AP,1` | Enable WiFi AP mode and reboot |
