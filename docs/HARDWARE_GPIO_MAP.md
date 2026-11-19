# Hardware and GPIO map — Smart Dustbin (ESP8266)

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Product baseline:** Simplified Smart Dustbin (WP-11.1+ approach-only, lid-linked white LED)  
**Last updated (docs):** 2026-11-19 (WP-11.6)

This document describes the **current physical configuration** and GPIO roles for firmware on the bench. It aligns with `SmartDustbin/pins.h` and `SmartDustbin/src/gpio_init.cpp`.

**Steven confirmation (WP-11.6):** Legacy waste sensor, yellow LED, and buzzer wiring is **physically disconnected or removed**. The buzzer is **removed from D4**. The active approach ECHO on **D2** uses the approved **1 kΩ / 2 kΩ** divider.

---

## 1. Controller

| Attribute | Value |
|-----------|--------|
| Module | LoLin NodeMCU V3 |
| MCU | ESP8266MOD / ESP-12E |
| USB–serial | CH340G (typical on V3) |
| IDE / CLI board | **NodeMCU 1.0 (ESP-12E Module)** — FQBN `esp8266:esp8266:nodemcuv2` |
| Flash | 4 MB |
| Serial (firmware) | 115200 baud |
| Typical PC port | Environment-specific |

---

## 2. GPIO summary — active vs unused

### 2.1 Active (current product)

| Function | NodeMCU | GPIO | Direction | Firmware symbol |
|----------|---------|------|-----------|-----------------|
| Approach TRIG | D1 | GPIO5 | OUTPUT | `PIN_APPROACH_TRIG` |
| Approach ECHO | D2 | GPIO4 | INPUT | `PIN_APPROACH_ECHO` |
| Servo signal | D7 | GPIO13 | OUTPUT (PWM) | `PIN_SERVO_SIGNAL` |
| White LED | D0 | GPIO16 | OUTPUT | `PIN_LED_WHITE` |

**White LED behaviour (firmware):** **ON while the lid is open**, OFF when closed (`white_indicator`).

### 2.2 Unused (legacy hardware removed or disconnected)

| NodeMCU | GPIO | Former use | Current state |
|---------|------|------------|---------------|
| D3 | GPIO0 | Yellow LED | **Disconnected** — not driven by firmware |
| D4 | GPIO2 | Buzzer | **Removed** — not configured in `gpio_init.cpp` |
| D5 | GPIO14 | Waste HC-SR04 TRIG | **Disconnected** |
| D6 | GPIO12 | Waste HC-SR04 ECHO | **Disconnected** |

**Firmware note:** `pins.h` still defines `PIN_BUZZER` on D4 for historical reference; **no** `pinMode` or drive on D4 at runtime. Removing the `#define` is a future firmware hygiene item, not required for correct operation today.

---

## 3. HC-SR04 approach sensor (single unit)

### 3.1 Wiring

| HC-SR04 pin | Connection |
|-------------|------------|
| VCC | 5 V (NodeMCU **VU** or approved 5 V rail) |
| GND | Common ground with NodeMCU |
| TRIG | **D1 / GPIO5** |
| ECHO | **D2 / GPIO4** via divider (§3.2) |

Only **one** HC-SR04 is used in the current product. There is **no** second sensor on D5/D6.

### 3.2 ECHO voltage divider (D2) — confirmed

HC-SR04 ECHO is **5 V** logic. ESP8266 GPIO is **3.3 V** tolerant only.

**Approved divider on the approach ECHO line:**

```text
ECHO (5 V) ----+----[ 1 kΩ ]----+----> D2 / GPIO4
               |                |
               +----[ 2 kΩ ]----> GND
```

Approximate tap: ~3.3 V at logic high (ratio 2/(1+2)).

**Do not** connect raw 5 V ECHO directly to the ESP8266.

---

## 4. SG90 servo (lid)

| Servo wire | Connection |
|------------|------------|
| Orange (signal) | **D7 / GPIO13** |
| Red (+5 V) | **External dedicated 5 V supply** (e.g. 5 V / 2 A adapter) |
| Brown (GND) | **External supply GND**, tied to **NodeMCU GND** |

### Power rules

- **Share ground only** between NodeMCU and external servo supply.
- **Do not** connect external servo **+5 V** to NodeMCU **VU** / USB 5 V rail.
- NodeMCU may remain on USB for logic and Wi-Fi while the servo runs on external 5 V.

### Angles

Firmware uses **provisional** open/closed angles in `config.h` (`CONFIG_SERVO_ANGLE_CLOSED` / `CONFIG_SERVO_ANGLE_OPEN`). Physical acceptance is recorded in [`CALIBRATION_LOG.md`](CALIBRATION_LOG.md) when measured — values are **not** treated as final until accepted there.

---

## 5. White LED (D0)

| Item | Value |
|------|------|
| Pin | **D0 / GPIO16** |
| Series resistor | **220 Ω** (typical; verify on bench) |
| Wiring | GPIO → resistor → LED anode → cathode → GND (verify polarity) |
| Firmware | HIGH = ON when lid open |

GPIO16 is not a flash boot strap pin in the same way as GPIO0/2/15; still use a sensible series resistor and verify polarity on assembly.

---

## 6. Boot-strapping reference (unused pins)

These constraints matter **if** loads are ever reconnected to D3/D4. They are **inactive** in the current physical build.

| GPIO | NodeMCU | Boot constraint | Current product |
|------|---------|-----------------|-----------------|
| GPIO0 | D3 | Must be HIGH for normal flash boot | Unused — yellow removed |
| GPIO2 | D4 | Must be HIGH for normal flash boot | Unused — buzzer removed |
| GPIO15 | D8 | Must be LOW at boot | Not allocated |
| GPIO16 | D0 | White LED | Active |

---

## 7. Grounding and commons

```text
                    ┌─────────────────┐
   USB / logic ──────│    NodeMCU      │
   5 V (VU) ────────│    ESP8266      │──── HC-SR04 VCC (5 V)
                    │    GND ─────────┼────┬── servo supply GND
                    └─────────────────┘    ├── HC-SR04 GND
                                           ├── White LED (−)
                                           └── servo brown
   External +5 V ─── (servo red only)      (no +5 V tie to VU)
```

All returns must share a **single common ground**.

---

## 8. Bench verification checklist (current product)

| ID | Topic | Status |
|----|--------|--------|
| HV-1 | D2 ECHO divider present; no 5 V on GPIO4 | Confirmed (WP-11.6) |
| HV-2 | Approach sensor ranging plausible | Ongoing calibration |
| HV-3 | Servo on external 5 V, shared GND only | Required |
| HV-4 | Legacy D3/D4/D5/D6 not connected | Confirmed removed/disconnected (WP-11.6) |
| HV-5 | White LED ON only when lid open | Firmware + bench |
| HV-6 | Servo angles suitable for physical lid | **Provisional** in `config.h` |

---

## Related documents

- [`CALIBRATION_LOG.md`](CALIBRATION_LOG.md) — thresholds and angles vs acceptance
- [`../README.md`](../README.md) — build, API, project layout
- [`WP-11.1_COMPLETION_REPORT.md`](WP-11.1_COMPLETION_REPORT.md) — removal of dual-sensor / fill hardware scope