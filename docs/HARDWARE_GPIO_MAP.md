# Hardware and GPIO map — Smart Dustbin (ESP8266)

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Status:** Approved baseline (Steven / Product Owner)  
**Last updated (docs):** 2026-11-12 (WP-0)

This document is the authoritative reference for **physical connections** and **GPIO roles**. It does not change wiring on the bench; it records what is approved for firmware and validation.

---

## 1. Controller

| Attribute | Value |
|-----------|--------|
| Module | LoLin NodeMCU V3 |
| MCU | ESP8266MOD / ESP-12E |
| USB–serial | CH340G (typical on V3) |
| IDE board selection | **NodeMCU 1.0 (ESP-12E Module)** |
| Flash | 4 MB |
| Serial (firmware default) | 115200 baud |
| Typical PC port | COM5 (environment-specific) |

---

## 2. GPIO allocation summary

| Function | NodeMCU label | ESP8266 GPIO | Direction | Notes |
|----------|---------------|--------------|-----------|--------|
| Approach sensor TRIG | D1 | GPIO5 | OUTPUT | 10 µs trigger pulse |
| Approach sensor ECHO | D2 | GPIO4 | INPUT | Via voltage divider |
| Waste-level sensor TRIG | D5 | GPIO14 | OUTPUT | 10 µs trigger pulse |
| Waste-level sensor ECHO | D6 | GPIO12 | INPUT | Via voltage divider |
| Servo signal | D7 | GPIO13 | OUTPUT (PWM) | Angle control |
| White LED (normal) | D0 | GPIO16 | OUTPUT | 220 Ω series resistor |
| Yellow LED (warning) | D3 | GPIO0 | OUTPUT | 220 Ω series resistor |
| Buzzer positive | D4 | GPIO2 | OUTPUT | Active HIGH; see §7 |

**Removed / not used:** Green LED (removed from build to free GPIO for buzzer).

---

## 3. HC-SR04 ultrasonic sensors (×2)

### 3.1 Approach (exterior / lid zone)

| HC-SR04 pin | Connection |
|-------------|------------|
| VCC | 5 V (NodeMCU **VU** or approved 5 V rail) |
| GND | Common ground with NodeMCU |
| TRIG | **D1 / GPIO5** |
| ECHO | **D2 / GPIO4** via divider (§3.3) |

### 3.2 Waste level (internal bin height)

| HC-SR04 pin | Connection |
|-------------|------------|
| VCC | 5 V (shared arrangement with approach sensor) |
| GND | Common ground |
| TRIG | **D5 / GPIO14** |
| ECHO | **D6 / GPIO12** via divider (§3.3) |

### 3.3 ECHO voltage dividers (each sensor)

HC-SR04 ECHO is **5 V** logic. ESP8266 GPIO is **3.3 V** tolerant only.

**Approved divider (per ECHO line, separate circuits):**

```text
ECHO (5 V) ----+----[ 1 kΩ ]----+----> to MCU ECHO pin (D2 or D6)
               |                |
               +----[ 2 kΩ ]----> GND
```

Approximate tap voltage: ~3.3 V at logic high (ratio 2/(1+2)).

**Do not** connect raw 5 V ECHO directly to the ESP8266.

### 3.4 Bench validation (Product Owner)

Individual sensors have produced plausible distance readings on the bench. Occasional unstable or distant readings remain subject to **filtering and calibration** in firmware (later work packages).

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
- NodeMCU may remain on USB for logic and Wi-Fi while servo runs on external 5 V.

### Mechanical / calibration

Servo has been exercised at **30°, 90°, and 150°** on the bench. Final **open** and **closed** lid angles are **TBD** — record in [`CALIBRATION_LOG.md`](CALIBRATION_LOG.md).

---

## 5. LEDs

Exactly **two** indicator states (no third LED):

| Condition | Indicator |
|-----------|-----------|
| Normal waste level | **White LED ON** (yellow off) |
| Warning / high | **Yellow LED ON** (white off) |

| LED | Pin | GPIO | Series resistor |
|-----|-----|------|-----------------|
| White | D0 | GPIO16 | 220 Ω |
| Yellow | D3 | GPIO0 | 220 Ω |

Typical wiring: GPIO → resistor → LED anode → cathode → GND (verify polarity on bench).

---

## 6. GPIO0 (D3) — boot-strapping

GPIO0 must be **HIGH** during reset for **normal SPI flash boot** (flash programming / run from flash).

**Yellow LED on D3 implications:**

- Firmware should configure the pin as output only after boot and avoid holding GPIO0 LOW during reset.
- LED circuit should be reviewed so it does not **force GPIO0 LOW** at power-on (e.g. accidental strong pull-down).

**Outstanding:** Capture schematic or photos and sign off cold-boot tests in a hardware validation work package.

---

## 7. Buzzer

**Approved classification (supersedes audit “unverified” wording — see audit addendum):**

| Attribute | Value |
|-----------|--------|
| Type | Two-terminal cylindrical buzzer (active/passive electrical class not formally documented) |
| Positive | **D4 / GPIO2** |
| Negative | **GND** |
| Driver | **Direct GPIO** (no transistor/MOSFET driver documented) |
| Activation | **GPIO2 HIGH** |
| Planned use | Short alert on transition **into** warning fill state + **cooldown** (firmware WP) |

**Bring-up:** Repository sketch `smart-dustbin-programme.ino` exercises D4 with HIGH and `tone()` for bench tests.

### GPIO2 (D4) — boot-strapping

GPIO2 must be **HIGH** during boot for normal operation.

**Outstanding electrical verification (required before production sign-off):**

- Cold-boot success with buzzer connected (10+ power cycles).
- GPIO source current vs buzzer draw (ESP8266 GPIO limit ~12 mA typical guidance — measure if in doubt).
- Whether passive vs active buzzer affects default pin level at reset.
- Long-term use of HIGH drive vs `tone()` for alert pattern.

Do **not** remove the buzzer from the architecture; validate and document results in hardware validation.

---

## 8. Grounding and commons

```text
                    ┌─────────────────┐
   USB / logic ──────│    NodeMCU      │
   5 V (VU) ────────│    ESP8266      │──── sensors VCC (5 V)
                    │    GND ─────────┼────┬── servo supply GND
                    └─────────────────┘    ├── HC-SR04 GND
                                           ├── LEDs, buzzer (−)
                                           └── servo brown
   External +5 V ─── (servo red only)      (no +5 V tie to VU)
```

All returns must meet at a **single common ground** reference.

---

## 9. Pin conflict and reserved pins (reference)

| GPIO | NodeMCU | Boot constraint |
|------|---------|-----------------|
| GPIO15 | D8 | Must be LOW at boot (unused in this map) |
| GPIO0 | D3 | HIGH for flash boot — yellow LED |
| GPIO2 | D4 | HIGH at boot — buzzer |
| GPIO16 | D0 | No flash-boot strap issue — white LED |

---

## 10. Outstanding electrical verification checklist

| ID | Topic | Owner | Status |
|----|--------|-------|--------|
| HV-1 | GPIO0 + yellow LED cold boot | Steven / bench | Open |
| HV-2 | GPIO2 + buzzer cold boot | Steven / bench | Open |
| HV-3 | Buzzer current at GPIO HIGH | Steven / DMM | Open |
| HV-4 | HC-SR04 cross-talk when both mounted | Firmware + bench | Open |
| HV-5 | Servo brown-out on lid load | Bench | Open |
| HV-6 | Divider tolerance on both ECHO lines | Visual / meter | Open |

---

## Related documents

- [`CALIBRATION_LOG.md`](CALIBRATION_LOG.md) — distances, angles, timings
- [`SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md`](SMARTDUSTBIN_AUDIT_ADDENDUM_CASE-20261112.md) — buzzer correction
- [`../README.md`](../README.md) — project overview
