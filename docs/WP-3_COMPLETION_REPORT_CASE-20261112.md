# WP-3 completion report — Waste-level sensing (dual HC-SR04 coordination)

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Work package:** WP-3  
**Date:** 2026-11-12  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-3 adds **raw waste-distance** measurement on **D5 TRIG / D6 ECHO** and introduces a **single shared ultrasonic scheduler** so approach and waste sensors **never trigger simultaneously**. A **60 ms acoustic quiet** window follows every measurement (complete or timeout).

WP-2 approach behaviour is preserved: **50 / 55 cm** thresholds, invalid handling, `approach_detected`, and **5 s Serial heartbeat** (extended with last valid waste cm).

No fill %, LEDs, servo, buzzer, Wi-Fi, or dashboard.

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/waste_sensor.h` | Waste reading API + last valid distance |
| `SmartDustbin/waste_sensor.cpp` | Waste HC-SR04 measure via common helper |
| `SmartDustbin/ultrasonic_scheduler.h` | Coordinated tick API |
| `SmartDustbin/ultrasonic_scheduler.cpp` | Single-sensor scheduling + quiet window |
| `SmartDustbin/hcsr04_common.h` | Shared reading struct + measure |
| `SmartDustbin/hcsr04_common.cpp` | One TRIG/ECHO/pulseIn path |
| `docs/WP-3_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/SmartDustbin.ino` | WP-3 banner, scheduler loop, `[waste]` output |
| `SmartDustbin/config.h` | Waste timing, `CONFIG_ULTRASONIC_QUIET_MS`, shared `CONFIG_HCSR04_US_PER_CM` |
| `SmartDustbin/approach_sensor.h/.cpp` | `approachSensorRunMeasurement()` only (no self-scheduling) |

### Unchanged

| Item |
|------|
| `smart-dustbin-programme/` legacy sketch |
| `pins.h` GPIO map |
| Buzzer GPIO not configured |

---

## 3. Waste sensor implementation

- Uses `hcsr04Measure()` on **PIN_WASTE_TRIG (D5)** and **PIN_WASTE_ECHO (D6)**.
- Timeout: `CONFIG_WASTE_PULSE_TIMEOUT_US` (30000 µs).
- Valid range: 2–400 cm (invalid never treated as valid distance).
- `wasteSensorGetLastValidDistance()` returns the most recent **valid** surface distance (held across invalid samples).
- Serial line format: `[waste] distance_cm=… valid=1` or `valid=0 reason=timeout|out_of_range`.

---

## 4. Measurement interval and timeout (defaults)

| Constant | Value |
|----------|--------|
| `CONFIG_WASTE_SAMPLE_MS` | **500** |
| `CONFIG_WASTE_PULSE_TIMEOUT_US` | **30000** |
| `CONFIG_WASTE_MIN_VALID_CM` / `MAX` | **2** / **400** |
| `CONFIG_APPROACH_SAMPLE_MS` | **100** (unchanged) |
| `CONFIG_APPROACH_DETECT_CM` / `CLEAR` | **50** / **55** (unchanged) |
| `CONFIG_ULTRASONIC_QUIET_MS` | **60** |
| `CONFIG_HCSR04_US_PER_CM` | **58** |

---

## 5. Ultrasonic coordination strategy

**Single entry point:** `ultrasonicSchedulerTick(nowMs)` in the main loop — no independent sensor schedulers.

**Rules:**

1. If `now < quietUntilMs`, no measurement starts.
2. A sensor is eligible when `now >= nextApproachMs` or `now >= nextWasteMs`.
3. If **both** are due, **alternating fairness** (`serveWasteNext` toggle) prevents starvation.
4. Run **exactly one** of `approachSensorRunMeasurement()` or `wasteSensorRunMeasurement()`.
5. After return: `quietUntilMs = millis() + 60`; reschedule the sensor that ran (`+100 ms` approach, `+500 ms` waste).

**Guarantee:** Only one TRIG pulse sequence active at a time; minimum **60 ms** between any two triggers.

---

## 6. Compilation result

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Result |
|--------|
| Exit code **0** (esp8266:esp8266 3.1.2) |

Upload: **not performed** (Steven).

---

## 7. Tests performed (agent)

| Test | Result |
|------|--------|
| Static: single scheduler calls both sensors | Pass |
| Static: no `approachSensorUpdate` duplicate path | Pass |
| Static: quiet interval constant 60 ms | Pass |
| Static: WP-4+ code absent | Pass |
| Build `arduino-cli compile` | **Pass** |
| On-device | Not performed |

---

## 8. Outstanding limitations

| ID | Note |
|----|------|
| L-1 | `pulseIn` still blocks up to timeout per scheduled shot |
| L-2 | Effective approach rate may be **< 10 Hz** when waste runs + quiet windows |
| L-3 | Waste **500 ms** + alternation may delay one sensor when both due together |
| L-4 | No fill % or calibration (WP-4+) |
| L-5 | Crosstalk reduction is logical only — bench validation still required |

---

## 9. Physical test instructions for Steven

**Setup:** Upload `SmartDustbin.ino`, **115200**, both HC-SR04 wired per `HARDWARE_GPIO_MAP.md`, buzzer disconnected.

| # | Test | Action | Expected |
|---|------|--------|----------|
| 1 | Boot | Reset | WP-3 banner; quiet 60 ms; sample intervals printed |
| 2 | Waste only | Approach sensor covered/disabled acoustically; vary bin height | `[waste]` ~every 500 ms (+ quiet); `valid=1` when aligned |
| 3 | Approach only | Focus on approach; ignore waste | `[approach]` ~100 ms cadence (modulo quiet); detect 50/55 cm behaviour |
| 4 | Both active | Hand approach + change waste surface | **Interleaved** `[approach]` and `[waste]` — **never simultaneous** (scope optional on TRIG pins) |
| 5 | Quiet gap | Logic probe D1 and D5 | ≥ **60 ms** between any two TRIG edges |
| 6 | Invalid waste | Block waste head | `valid=0`; heartbeat `waste_last_valid_cm` holds last good value |
| 7 | Heartbeat | Wait 5 s | `approach_detected` + `waste_last_valid_cm` |

Document findings in `CALIBRATION_LOG.md` when ready.

---

## 10. Skill selection

`atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not present**. **No skill used.**

---

**Stop:** Await Steven’s physical evidence and Craig’s review before WP-4.

*End of WP-3 completion report.*
