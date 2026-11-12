# WP-2 completion report — Approach detection subsystem

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Work package:** WP-2  
**Date:** 2026-11-12  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E  
**Agent:** Cursor (Implementation Agent)

---

## 1. Implementation summary

WP-2 adds a modular **HC-SR04 approach sensor** driver that:

- Triggers and reads ECHO with a **bounded** `pulseIn` timeout.
- Converts round-trip pulse time to **distance (cm)** via `config.h` constants.
- Rejects **timeouts** and **out-of-range** distances.
- Maintains **`approach_detected`** with **hysteresis** (detect / clear thresholds in `config.h`).
- Prints `[approach]` lines and retains **non-blocking** `[heartbeat]` every 5 s on Serial @ 115200.

**Excluded per assignment:** servo, waste sensor, LEDs, buzzer GPIO, Wi-Fi, dashboard, persistence.

**Buzzer / D4:** Not initialised in `gpio_init.cpp` (physically disconnected).

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/approach_sensor.h` | Reading struct and public API |
| `SmartDustbin/approach_sensor.cpp` | Measurement, validation, approach state |
| `docs/WP-2_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/SmartDustbin.ino` | WP-2 loop: `approachSensorUpdate`, Serial output, banner |
| `SmartDustbin/config.h` | Approach thresholds, interval, timeout, valid range |
| `SmartDustbin/gpio_init.cpp` | Removed buzzer pin configuration |

### Unchanged

| Item | Note |
|------|------|
| `smart-dustbin-programme/smart-dustbin-programme.ino` | Legacy diagnostic preserved |
| `pins.h` | Same approved map (buzzer pin defined but not driven) |
| Waste TRIG/ECHO, servo, LEDs | Idle init only (WP-1 behaviour) |

---

## 3. GPIO configuration used

| Signal | Pin | GPIO | Role in WP-2 |
|--------|-----|------|----------------|
| Approach TRIG | D1 | 5 | OUTPUT — 10 µs trigger pulse |
| Approach ECHO | D2 | 4 | INPUT — ECHO via divider |
| Waste TRIG | D5 | 14 | OUTPUT LOW (idle) |
| Waste ECHO | D6 | 12 | INPUT (unused) |
| Servo | D7 | 13 | OUTPUT LOW (idle) |
| White LED | D0 | 16 | OUTPUT LOW |
| Yellow LED | D3 | 0 | OUTPUT LOW |
| Buzzer | D4 | 2 | **Not configured** |

---

## 4. Detection threshold and measurement defaults

| Constant | Default | Notes |
|----------|---------|--------|
| `CONFIG_APPROACH_DETECT_CM` | **50** | Provisional — not final calibration |
| `CONFIG_APPROACH_CLEAR_CM` | **55** | Hysteresis to reduce flapping |
| `CONFIG_APPROACH_SAMPLE_MS` | **100** | Min interval between triggers |
| `CONFIG_APPROACH_PULSE_TIMEOUT_US` | **30000** | `pulseIn` bound (~5 m theoretical) |
| `CONFIG_APPROACH_MIN_VALID_CM` | **2** | Below → invalid |
| `CONFIG_APPROACH_MAX_VALID_CM` | **400** | Above → invalid |
| `CONFIG_APPROACH_US_PER_CM` | **58** | HC-SR04 round-trip conversion |
| `CONFIG_HEARTBEAT_MS` | **5000** | Diagnostic heartbeat retained |

**State logic:** Invalid readings **never** set `approach_detected` true. On **valid** readings: `<= 50 cm` → detected; `>= 55 cm` → clear; between → hold previous state.

---

## 5. Compilation command and result

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Tool | Version |
|------|---------|
| arduino-cli | 1.5.1 (Arduino IDE bundle) |
| Core | esp8266:esp8266 **3.1.2** |

**Result:** Exit code **0** (success).

No extra libraries beyond ESP8266 Arduino Core.

**Upload:** Not performed (Steven).

---

## 6. Tests performed

| Test type | What was done |
|-----------|----------------|
| Static review | Pin map vs `HARDWARE_GPIO_MAP.md`; no buzzer GPIO; no WP-3+ code |
| Build | `arduino-cli compile` — **pass** |
| On-device / bench | **Not performed** by agent |

---

## 7. Known limitations and outstanding risks

| ID | Limitation |
|----|------------|
| R-1 | `pulseIn` **blocks** up to `CONFIG_APPROACH_PULSE_TIMEOUT_US` per sample — acceptable at 100 ms interval but not fully non-blocking |
| R-2 | 50 / 55 cm defaults are **development** values only |
| R-3 | Only one ultrasonic active — waste TRIG remains LOW (no WP-3 crosstalk yet) |
| R-4 | `out_of_range` reports raw computed cm for debug — still **invalid** for detection |
| R-5 | ECHO divider and acoustic noise not validated in software |

---

## 8. Physical testing instructions for Steven

**Prerequisites**

- Board: **NodeMCU 1.0 (ESP-12E Module)**
- Open `SmartDustbin/SmartDustbin.ino`, compile, upload to **COM5** (or your port)
- Serial Monitor: **115200** baud
- Approach HC-SR04 on **D1/D2** only; buzzer **disconnected** per WP-2

**Expected boot**

- Banner with detect/clear cm and sample interval
- `[boot] GPIO safe idle + approach sensor ready.`
- Periodic `[approach]` lines ~every **100 ms** when running

| # | Test | Action | Expected Serial behaviour |
|---|------|--------|----------------------------|
| 1 | Initialisation | Power on after upload | Boot banner; no crash |
| 2 | No object near | Empty field in front of approach sensor | `distance_cm` typically > 50 (room dependent); `detected=0` |
| 3 | Approach | Move hand/object toward sensor within ~50 cm | Valid `distance_cm` decreases; `detected=1` when ≤ 50 cm |
| 4 | Retreat | Move away beyond ~55 cm | `detected=0` after valid read ≥ 55 cm |
| 5 | Distances | Use tape measure at ~80, 50, 30 cm | Compare printed `distance_cm` (rough check) |
| 6 | Timeout / invalid | Block sensor, point at void, or unplug ECHO briefly | `valid=0 reason=timeout` or `out_of_range`; **must not** show `detected=1` from invalid lines alone |
| 7 | Heartbeat | Wait 5 s | `[heartbeat]` includes `approach_detected=` matching state |

**Record** results (dates, notes) in `docs/CALIBRATION_LOG.md` §1 when values are accepted.

**Tuning:** Adjust `CONFIG_APPROACH_DETECT_CM` / `CLEAR_CM` / `SAMPLE_MS` in `config.h` if flapping or latency issues appear (recompile/upload).

---

## 9. Skill selection

**Path:** `atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not present**.

**Selected skill:** None.

---

## 10. WP-2 acceptance checklist

| Criterion | Met |
|-----------|-----|
| Approach sensor on D1/D2 | Yes |
| Timeout and invalid handling | Yes |
| `approach_detected` on Serial | Yes |
| Thresholds in `config.h` (50 cm default) | Yes |
| No excluded features | Yes |
| Buzzer not configured | Yes |
| Legacy sketch unchanged | Yes |
| Compiles with nodemcuv2 | Yes |
| No commit/push | Yes |

---

**Stop:** Await Steven’s physical validation before WP-3.

*End of WP-2 completion report.*
