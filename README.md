# Smart IoT Dustbin (ESP8266)

Intelligent waste bin firmware for **LoLin NodeMCU V3 (ESP8266MOD)** with approach detection, automatic lid control, internal fill monitoring, local indicators, buzzer alerts on warning transitions, and a **monitoring-only** web dashboard served on the local network.

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Build environment:** **Arduino IDE only** (no PlatformIO).

---

## Development status

| Area | Status |
|------|--------|
| Repository documentation (WP-0) | **Complete** |
| Integrated product firmware | **WP-5** — approach lid + fill % (`SmartDustbin/`) |
| Buzzer test sketch | Present (`smart-dustbin-programme/`) — diagnostic only (preserved) |
| Sensors, servo, LEDs, fill logic | **Planned** |
| Wi-Fi, HTTP API, on-device dashboard | **Planned** |
| Physical calibration values | **TBD** — see [`docs/CALIBRATION_LOG.md`](docs/CALIBRATION_LOG.md) |

Do **not** assume approach detection, automatic lid operation, fill percentage, LED level indication, buzzer warning alerts, or the web dashboard are working in the current firmware unless a later work package explicitly delivers them.

---

## Controller and tools

| Item | Value |
|------|--------|
| Board | LoLin NodeMCU V3 (ESP-12E module) |
| Arduino IDE board | **NodeMCU 1.0 (ESP-12E Module)** |
| Core | ESP8266 Arduino Core |
| Typical upload port | COM5 (adjust to your PC) |
| Serial monitor | **115200** baud |
| Flash | 4 MB |

Install the **ESP8266** board package in Arduino IDE (*Boards Manager*) before building.

---

## Hardware inventory

- 2× HC-SR04 ultrasonic sensors (approach + internal waste level)
- 1× SG90 micro servo (lid)
- 1× White LED (normal level)
- 1× Yellow LED (warning / high level)
- 1× Two-pin buzzer (warning transition alert)
- Breadboard and jumper wires
- External **5 V** supply for the servo (ground shared with NodeMCU; **do not** tie servo +5 V to NodeMCU USB/VU)

Full wiring and boot notes: [`docs/HARDWARE_GPIO_MAP.md`](docs/HARDWARE_GPIO_MAP.md).

---

## Approved GPIO allocation

| Component | NodeMCU | GPIO |
|-----------|---------|------|
| Approach TRIG | D1 | 5 |
| Approach ECHO | D2 | 4 |
| Waste TRIG | D5 | 14 |
| Waste ECHO | D6 | 12 |
| Servo signal | D7 | 13 |
| White LED | D0 | 16 |
| Yellow LED | D3 | 0 |
| Buzzer (+) | D4 | 2 |

Each HC-SR04 ECHO line uses a **1 kΩ / 2 kΩ** divider (~3.3 V at the MCU). Servo red → external 5 V; brown → supply GND; orange → D7.

---

## Buzzer (approved baseline)

- **Wiring:** Positive → **D4 / GPIO2**; negative → **GND** (direct GPIO, no separate driver documented).
- **Activation:** GPIO **HIGH** (active buzzer behaviour; bring-up sketch also exercises `tone()` for diagnostics).
- **Product behaviour (planned):** Short beep when fill level **enters** warning state, with a cooldown.
- **Boot note:** GPIO2 is an ESP8266 boot-strapping pin (must be HIGH during boot for normal flash boot). Electrical load, GPIO current limits, and reliable cold-start behaviour must be **validated in a later hardware work package** — see hardware doc and audit addendum.

---

## Power arrangement

| Load | Supply |
|------|--------|
| NodeMCU | USB (or approved 3.3 V / USB power) |
| HC-SR04 | 5 V (e.g. VU) with common GND |
| Servo | **Dedicated external 5 V** (e.g. phone charger); GND tied to NodeMCU GND only |
| LEDs / buzzer | From NodeMCU GPIO (with series resistor on LEDs — 220 Ω each) |

Never connect the servo’s **+5 V** rail to the NodeMCU **VU** output.

---

## Planned software architecture

Approved direction (implementation arrives in later work packages):

```text
smart-dustbin-programme/
├── smart-dustbin-programme.ino   # setup(), loop(), thin orchestration
├── config.h                      # Default timings/thresholds (no secrets)
├── pins.h                        # GPIO map (matches hardware doc)
├── approach_sensor.*             # HC-SR04 #1
├── fill_sensor.*                 # HC-SR04 #2
├── lid_servo.*                   # Non-blocking lid state machine
├── level_leds.*                  # White / yellow indicators
├── buzzer.*                      # Warning transition + cooldown
├── filter.*                      # Shared distance filtering
├── waste_level.*                 # Fill % from calibrated distances
├── network.* / telemetry.*       # Wi-Fi + HTTP API
├── dashboard/                    # Static assets served by ESP8266
└── secrets.h                     # Wi-Fi credentials — local only, gitignored
```

- **Dashboard:** Hosted on the ESP8266; open `http://<device-ip>/` on the LAN — **monitoring only** for the prototype.
- **Wi-Fi:** Credentials in a **local file** (e.g. `secrets.h`) — never committed — see `.gitignore`.
- **Calibration:** Empty/full distances, thresholds, and servo angles recorded in [`docs/CALIBRATION_LOG.md`](docs/CALIBRATION_LOG.md).

WP-0 does **not** create these modules or move the sketch; that begins at **WP-1** after approval.

---

## Arduino IDE — build and upload

**Production foundation sketch (WP-1+):** `SmartDustbin/SmartDustbin.ino`

**Legacy buzzer diagnostic:** `smart-dustbin-programme/smart-dustbin-programme.ino`

1. Open the sketch folder: `SmartDustbin/SmartDustbin.ino` (File → Open).
2. Select board: **NodeMCU 1.0 (ESP-12E Module)**.
3. Select the correct **Port** (e.g. COM5).
4. **Upload** (→).
5. Open **Serial Monitor** at **115200** baud.

No extra libraries are required for the current buzzer test sketch. Future work packages will document any added libraries in the README and completion reports.

---

## Documentation

| Document | Purpose |
|----------|---------|
| [`docs/SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md`](docs/SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md) | Stage-1 audit (historical baseline) |
| [`docs/SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md`](docs/SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md) | Buzzer classification correction |
| [`docs/HARDWARE_GPIO_MAP.md`](docs/HARDWARE_GPIO_MAP.md) | Authoritative GPIO and power wiring |
| [`docs/CALIBRATION_LOG.md`](docs/CALIBRATION_LOG.md) | Physical calibration record |
| [`docs/WP-0_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-0_COMPLETION_REPORT_CASE-20261112.md) | WP-0 delivery evidence |
| [`docs/WP-1_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-1_COMPLETION_REPORT_CASE-20261112.md) | WP-1 firmware foundation evidence |
| [`docs/WP-2_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-2_COMPLETION_REPORT_CASE-20261112.md) | WP-2 approach sensor evidence |
| [`docs/WP-3_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-3_COMPLETION_REPORT_CASE-20261112.md) | WP-3 waste sensor + scheduler evidence |
| [`docs/WP-4_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-4_COMPLETION_REPORT_CASE-20261112.md) | WP-4 fill percentage evidence |
| [`docs/WP-5_COMPLETION_REPORT_CASE-20261112.md`](docs/WP-5_COMPLETION_REPORT_CASE-20261112.md) | WP-5 automatic lid control evidence |

---

## Repository layout (current)

```text
SMART-DUSTBIN-ESP8266MOD/
├── README.md
├── .gitignore
├── SmartDustbin/
│   ├── SmartDustbin.ino
│   ├── config.h
│   ├── pins.h
│   ├── gpio_init.cpp / gpio_init.h
│   └── wifi_config.h.example
├── docs/
├── smart-dustbin-programme/
│   └── smart-dustbin-programme.ino
└── SALLE KAMANISHA ... PROJECT PROPOSAL..7z
```

---

## License and attribution

Academic IoT project — see proposal archive in repository root.
