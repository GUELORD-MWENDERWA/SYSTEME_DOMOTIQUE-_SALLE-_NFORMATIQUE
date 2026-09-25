# Computer Lab Automation System (ESP32)

An ESP32-based building automation controller for a computer lab. It combines RFID access control with occupancy counting, energy metering, daylight-aware lighting, intrusion detection and remote control through a web dashboard, a REST API and a serial console.

## Features

- **Access control**: two MFRC522 readers (entry and exit), servo-driven gate, authorized badges stored in EEPROM, on-device enrollment mode
- **Occupancy tracking**: entry and exit counters that drive lighting and security logic
- **Energy monitoring**: voltage, current, power and cumulative energy from a PZEM-004T v3 meter
- **Smart lighting**: indoor and outdoor lamps controlled from an LDR day/night detector with hysteresis
- **Security**: intrusion alarm when motion is detected at night while the room is empty
- **Load control**: 8 outputs through a 74HC595 shift register (2 lamps, 2 sockets, fan, buzzer, red and green LEDs)
- **Interfaces**: 16x2 LCD status screen, web dashboard served from SPIFFS, REST API, serial command console
- **Connectivity**: station mode with credentials stored in EEPROM, automatic reconnection, and a fallback configuration access point

## System overview

```
            ┌──────────────┐     ┌────────────────┐
 RC522 x2 ──┤              │     │  74HC595       │── lamps, sockets, fan,
 PZEM-004T ─┤    ESP32     ├─────┤  shift register│   buzzer, LEDs
 LDR, PIR ──┤  (FreeRTOS,  │     └────────────────┘
 Buttons  ──┤  Arduino)    ├── Servo (gate)
            │              ├── LCD 16x2 (I2C)
            └──────┬───────┘
                   │ Wi-Fi
         Web dashboard  /  REST API  /  Serial console
```

## Hardware

Pin assignments are centralized in [`src/config.h`](src/config.h):

| Peripheral | Pins |
| --- | --- |
| PZEM-004T (UART2, 9600 baud) | RX 32, TX 33 |
| RFID RC522 x2 (SPI) | CLK 18, MISO 19, MOSI 23, RST 16, SS 17 and 5 |
| 74HC595 | LATCH 27, CLOCK 14, DATA 13 |
| Gate servo | 25 |
| LCD 16x2 I2C (`0x27`) | SDA 21, SCL 22 |
| Mode button (long press) / lamp button | 4 / 15 |
| LDR | 35 (ADC) |
| PIR motion sensor | 12 |

## Software architecture

Each subsystem is an independent module under `src/`:

| Module | Responsibility |
| --- | --- |
| `energy` | PZEM-004T polling and reporting |
| `rfid` | Dual-reader management, badge registry |
| `relay` | 74HC595 output driver |
| `servo` | Gate control |
| `ldr`, `motion` | Environmental sensing with debouncing |
| `logic` | Scenario engine (day/night, occupancy, intrusion) |
| `signaling` | Buzzer and LED patterns |
| `lcd` | Status display |
| `storage` | EEPROM persistence (badges, Wi-Fi) |
| `serial_cmd` | Serial console |
| `web` | Async HTTP server and REST API |

## REST API

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/api/system` | System state and energy readings |
| `POST` | `/api/relay?id=<n>` | Toggle an output |
| `GET` | `/api/rfid/list` | Authorized badges |
| `POST` | `/api/rfid/register` | Register a badge |
| `POST` | `/api/rfid/delete?uid=<uid>` | Remove a badge |
| `POST` | `/api/cmd?cmd=<ZERO\|RESET\|RELAY\|REBOOT>` | System commands |
| `GET` / `POST` | `/api/config/wifi` | Read or update Wi-Fi settings |
| `GET` | `/api/files` | SPIFFS listing (diagnostics) |

Full details and examples: [API_ENDPOINTS.md](API_ENDPOINTS.md).

## Serial console

115200 baud. Main commands:

| Group | Commands |
| --- | --- |
| Energy | `E` (live readings), `ER` (report) |
| RFID | `RLIST`, `RMODE`, `RREG`, `RACC`, `RDEL`, `RNAME`, `REDIT` |
| Outputs | `L0 1/0`, `L1 1/0`, `P1 1/0`, `P2 1/0`, `F 1/0` |
| Diagnostics | `D` (state), `T` (self-test), `C` (occupancy counters) |
| System | `INFO`, `HELP`, `REBOOT` |

## Operating scenarios

| Condition | Behaviour |
| --- | --- |
| Authorized badge at entry | Gate opens for 3 s, green LED, short beep, occupancy +1, indoor light on |
| Unknown badge | Gate stays closed, red LED for 3 s, two short beeps |
| Last person leaves | Occupancy reaches 0, indoor light off |
| Night, room empty, motion detected | Intrusion alarm: outdoor light on, red LED, repeated long beeps |
| Mode button held for 2 s | Badge enrollment mode |

## Getting started

```bash
pio run -t upload        # firmware
pio run -t uploadfs      # web dashboard (SPIFFS)
pio device monitor       # serial console, 115200 baud
```

On first boot the controller tries the Wi-Fi credentials stored in EEPROM (or the defaults in `config.h`). If it cannot connect, it starts the `Domotique_Setup` access point; browse to `http://192.168.4.1/config.html` to configure the network.

Change the default Wi-Fi and access point passwords before deploying the system.

## Documentation

| Document | Content |
| --- | --- |
| [API_ENDPOINTS.md](API_ENDPOINTS.md) | REST API reference |
| [GUIDE_DEBUG_HARDWARE.md](GUIDE_DEBUG_HARDWARE.md) | Hardware troubleshooting |
| [CHECKLIST_VALIDATION.md](CHECKLIST_VALIDATION.md) | Acceptance test checklist |
| [AMELIORATIONS.md](AMELIORATIONS.md) | Planned improvements |
| [SUMMARY_FIXES.md](SUMMARY_FIXES.md), [CORRECTIONS_SIGNALING.md](CORRECTIONS_SIGNALING.md) | Change log of fixes |

## Safety

Mains-powered loads must be switched through properly rated relays, fused, and installed in an enclosure by a qualified electrician.

## License

No license has been specified yet. Contact the author before reusing this code.
