# Smart IoT Dustbin (ESP8266)

Approach-sensing **automatic lid** firmware for **LoLin NodeMCU V3 (ESP8266MOD)** with a **white indicator** (on while the lid is open), **Wi-Fi**, and a **monitoring-only** web dashboard on the local network.

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Build:** Arduino IDE or Arduino CLI (ESP8266 core — no PlatformIO).

---

## What the product does today

| Feature | Status |
|---------|--------|
| HC-SR04 approach detection (single sensor) | **Implemented** |
| Automatic SG90 lid open / close with close delay | **Implemented** |
| White LED on D0 — **ON while lid is open** | **Implemented** |
| Wi-Fi + HTTP on port **80** | **Implemented** |
| Dashboard at `http://<device-ip>/` | **Implemented** |
| JSON `GET /api/status`, `GET /api/health` | **Implemented** |
| Waste-level sensor, fill %, yellow LED, buzzer | **Removed** (WP-11.1+) |

Thresholds and servo angles in `config.h` are **provisional development defaults** until recorded as **Accepted** in [`docs/CALIBRATION_LOG.md`](docs/CALIBRATION_LOG.md).

---

## Controller and tools

| Item | Value |
|------|--------|
| Board | LoLin NodeMCU V3 (ESP-12E module) |
| Arduino IDE board | **NodeMCU 1.0 (ESP-12E Module)** |
| Arduino CLI FQBN | `esp8266:esp8266:nodemcuv2` |
| Core | ESP8266 Arduino Core (install via Boards Manager) |
| Serial monitor | **115200** baud (`CONFIG_SERIAL_BAUD`) |
| Typical upload port | Environment-specific (e.g. COM5) |
| Flash | 4 MB |

**Libraries** (via ESP8266 core — no extra install for this sketch): `Servo`, `ESP8266WiFi`, `ESP8266WebServer`.

---

## Hardware inventory (current build)

| Part | Role |
|------|------|
| 1× HC-SR04 | Approach / proximity |
| 1× SG90 micro servo | Lid |
| 1× White LED + series resistor (see hardware doc) | Lid-open indicator |
| NodeMCU + breadboard / wiring | Controller |
| External **5 V** supply | **Servo only** (shared GND with NodeMCU) |

**Not used in the current product** (legacy wiring removed on bench — see Steven confirmation in WP-11.6):

- Second HC-SR04 (former D5/D6)
- Yellow LED (D3)
- Buzzer (D4)

Full wiring: [`docs/HARDWARE_GPIO_MAP.md`](docs/HARDWARE_GPIO_MAP.md).

---

## Active GPIO map

| Function | NodeMCU | GPIO | Firmware |
|----------|---------|------|----------|
| Approach TRIG | D1 | 5 | `PIN_APPROACH_TRIG` |
| Approach ECHO | D2 | 4 | `PIN_APPROACH_ECHO` (via 1 kΩ / 2 kΩ divider) |
| Servo signal | D7 | 13 | `PIN_SERVO_SIGNAL` |
| White LED | D0 | 16 | `PIN_LED_WHITE` |

### Unused in current physical configuration

| NodeMCU | GPIO | Notes |
|---------|------|--------|
| D3 | 0 | Legacy yellow LED — disconnected |
| D4 | 2 | Legacy buzzer — **removed**; `PIN_BUZZER` still in `pins.h` but **not** configured in `gpio_init.cpp` |
| D5 | 14 | Legacy waste TRIG — disconnected |
| D6 | 12 | Legacy waste ECHO — disconnected |

---

## Repository layout

```text
SMART-DUSTBIN-ESP8266MOD/
├── README.md
├── .gitignore
├── docs/                          # Audits, calibration log, WP reports
├── SmartDustbin/                  # Arduino sketch (open this folder in the IDE)
│   ├── SmartDustbin.ino           # setup(), loop(), orchestration
│   ├── config.h                   # Timings and thresholds (provisional values)
│   ├── pins.h                     # GPIO defines
│   ├── wifi_config.h.example      # Template — copy to wifi_config.h
│   ├── wifi_config.h              # Local only (gitignored)
│   └── src/                       # Firmware modules (.h / .cpp)
│       ├── approach_sensor.*
│       ├── hcsr04_common.*
│       ├── lid_control.*
│       ├── white_indicator.*
│       ├── application_status.*
│       ├── wifi_manager.*
│       ├── http_api.*
│       ├── dashboard_page.*       # Embedded HTML (PROGMEM)
│       └── gpio_init.*
└── … (course proposal archive at repo root)
```

The main `.ino` includes modules with `#include "src/module.h"`. Module `.cpp` files include sketch-root headers as `#include "../config.h"` / `../pins.h` / `../wifi_config.h` (ESP8266 build paths).

---

## Module responsibilities

| Module | Responsibility |
|--------|----------------|
| `SmartDustbin.ino` | Init order, loop scheduling, approach sample timing |
| `gpio_init` | Safe pin modes before subsystems start |
| `hcsr04_common` / `approach_sensor` | Ultrasonic ranging and approach hysteresis |
| `lid_control` | Servo lid state machine and close delay |
| `white_indicator` | White LED follows lid open/closed |
| `application_status` | Snapshot for Serial / HTTP |
| `wifi_manager` | Non-blocking Wi-Fi connect / reconnect |
| `http_api` | Routes, JSON builders |
| `dashboard_page` | Dashboard HTML string |

---

## Wi-Fi configuration

1. Copy `SmartDustbin/wifi_config.h.example` → `SmartDustbin/wifi_config.h` (same folder as the `.ino`).
2. Set `WIFI_SSID` and `WIFI_PASSWORD`.
3. `wifi_config.h` is **gitignored** — never commit credentials.

Reconnect interval: `CONFIG_WIFI_RECONNECT_INTERVAL_MS` in `config.h`.

---

## Build and upload

### Arduino IDE

1. **File → Open** → folder `SmartDustbin` (not the repo root).
2. **Tools → Board** → **NodeMCU 1.0 (ESP-12E Module)**.
3. Select the correct **Port**.
4. Ensure `wifi_config.h` exists locally.
5. **Upload**, then **Serial Monitor** at **115200**.

### Arduino CLI

From the **repository root**:

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

Install the ESP8266 platform first (`arduino-cli core install esp8266:esp8266`).

---

## Dashboard and HTTP API

**Monitoring only** — no remote control of lid, servo, or LED from the network.

| Route | Purpose |
|-------|---------|
| `GET /` | Dashboard HTML (embedded in firmware) |
| `GET /api/status` | Live device snapshot (JSON) |
| `GET /api/health` | Liveness + Wi-Fi + `monitoring_only: true` |

**Dashboard:** On the LAN, open `http://<device-ip>/` (port 80).

**Client poll interval:** 2500 ms (`POLL_MS` in `dashboard_page.cpp`).

### `/api/status` (summary)

| JSON path | Meaning |
|-----------|---------|
| `device.uptime_ms` | Milliseconds since boot |
| `approach.detected` | Approach sensor latched “nearby” |
| `lid.state` | `"OPEN"` or `"CLOSED"` |
| `lid.close_delay_pending` | Lid will close after delay when clear |
| `lid.servo_target_angle_deg` | Servo target angle |
| `leds.white_on` | White indicator state |
| `network.wifi_connected` | Wi-Fi up |
| `network.ip` | Local IP or `null` |

### `/api/health` (summary)

| JSON path | Meaning |
|-----------|---------|
| `device.status` | `"ok"` |
| `device.uptime_ms` | Uptime |
| `wifi.connected` / `wifi.ip` | Wi-Fi state |
| `features.monitoring_only` | Always `true` |

---

## Documentation index

### Current product (WP-11 simplification + structure)

| Document | Purpose |
|----------|---------|
| [`docs/HARDWARE_GPIO_MAP.md`](docs/HARDWARE_GPIO_MAP.md) | Wiring and GPIO (current build) |
| [`docs/CALIBRATION_LOG.md`](docs/CALIBRATION_LOG.md) | Physical calibration vs firmware defaults |
| [`docs/WP-11_SIMPLIFICATION_IMPACT_AUDIT.md`](docs/WP-11_SIMPLIFICATION_IMPACT_AUDIT.md) | WP-11 scope |
| [`docs/WP-11.1_COMPLETION_REPORT.md`](docs/WP-11.1_COMPLETION_REPORT.md) | Waste monitoring removal |
| [`docs/WP-11.2_COMPLETION_REPORT.md`](docs/WP-11.2_COMPLETION_REPORT.md) | Lid-linked white LED |
| [`docs/WP-11.3_COMPLETION_REPORT.md`](docs/WP-11.3_COMPLETION_REPORT.md) | API / dashboard simplification |
| [`docs/WP-11.4_COMPLETION_REPORT.md`](docs/WP-11.4_COMPLETION_REPORT.md) | Dashboard UX |
| [`docs/WP-11.5_STRUCTURE_COMPLETION_REPORT.md`](docs/WP-11.5_STRUCTURE_COMPLETION_REPORT.md) | `src/` project layout |
| [`docs/WP-11.6_DOCUMENTATION_AUDIT.md`](docs/WP-11.6_DOCUMENTATION_AUDIT.md) | Documentation gap analysis |
| [`docs/WP-11.6_DOCUMENTATION_COMPLETION_REPORT.md`](docs/WP-11.6_DOCUMENTATION_COMPLETION_REPORT.md) | WP-11.6 documentation delivery |
| [`docs/CASE-20261119-DUSTBIN-LED-D0-INVESTIGATION.md`](docs/CASE-20261119-DUSTBIN-LED-D0-INVESTIGATION.md) | **Historical** LED investigation (superseded) |

### Historical delivery records (WP-0–WP-9)

These describe earlier milestones (dual sensor, fill logic, yellow LED, etc.). They are **not** the current product specification:

- [`docs/WP-0_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-0_COMPLETION_REPORT_CASE-20261112.md) through [`docs/WP-9_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-9_COMPLETION_REPORT_CASE-20261112.md)
- [`docs/SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md`](docs/SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md)
- [`docs/SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md`](docs/SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md)

**Legacy buzzer sketch** `smart-dustbin-programme/` is **not** in this repository.

---

## License and attribution

Academic IoT project — see course proposal material in the repository root.