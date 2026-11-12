# Calibration log — Smart Dustbin (ESP8266)

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Purpose:** Record **physical measurements** and acceptance for firmware constants.  
**Rule:** Do **not** invent values. Use **TBD** until measured on hardware. Copy a row template for each new test session.

---

## How to use this log

1. Measure on the **assembled bin** under realistic conditions (lid state, liner, room noise, etc.).
2. Fill in **Test date**, **Test conditions**, **Measured value**, **Observations**, and **Acceptance** (`Pending` / `Accepted` / `Rejected`).
3. When a value is **Accepted**, mirror it into firmware `config.h` (later work package) and note the git tag or commit in **Observations**.
4. Leave prior rows unchanged for traceability; add new rows when re-calibrating.

---

## 1. Approach sensor — detection distance

**Meaning:** Distance at which a person/object is considered “approaching” for lid open (after filtering/debounce in firmware).

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Approach trigger distance | `APPROACH_DETECT_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Approach clear / farther distance (hysteresis) | `APPROACH_CLEAR_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Min retrigger time | `APPROACH_RETRIGGER_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 2. Waste sensor — empty and full references

**Meaning:** Distances from waste-level HC-SR04 to waste surface. Used to compute fill percentage:

\[
\text{fill \%} = f(\text{distance}, \text{empty\_cm}, \text{full\_cm})
\]

(Exact formula to be fixed in firmware WP; typically linear map between empty and full.)

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Empty bin reference (surface far / min fill) | `WASTE_EMPTY_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Full bin reference (surface near / max fill) | `WASTE_FULL_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Invalid reading timeout | `HCSR04_TIMEOUT_US` | µs | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Max valid distance clamp | `WASTE_MAX_VALID_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 3. Waste-level warning threshold and hysteresis

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Warning threshold (% fill) | `WARN_FILL_PERCENT` | % | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Clear warning hysteresis (% points) | `WARN_HYSTERESIS_PERCENT` | % | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Optional warning threshold (distance) | `WARN_DISTANCE_CM` | cm | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 4. Servo — lid angles

Bench tests reference 30°, 90°, 150°; **open** and **closed** product angles are **TBD**.

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Lid closed angle | `SERVO_ANGLE_CLOSED` | degrees | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Lid open angle | `SERVO_ANGLE_OPEN` | degrees | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Servo step / move duration limit | `SERVO_MOVE_TIMEOUT_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 5. Lid open duration (dwell)

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Minimum lid open time after approach | `LID_OPEN_DWELL_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Lid close delay after clear approach | `LID_CLOSE_DELAY_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 6. Sensor sampling intervals

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Approach sample period | `APPROACH_SAMPLE_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Waste level sample period | `FILL_SAMPLE_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Cross-sensor guard time (anti crosstalk) | `ULTRASONIC_GUARD_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Filter window (e.g. median N) | `DISTANCE_FILTER_N` | samples | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 7. Buzzer alert

| Parameter | Firmware key (planned) | Target unit | Value | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------------------------|-------------|-------|-----------|-----------------|----------------|--------------|------------|
| Alert duration (warning entry) | `BUZZER_ALERT_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Cooldown between alerts | `BUZZER_COOLDOWN_MS` | ms | **TBD** | TBD | TBD | TBD | TBD | Pending |
| Drive mode (HIGH vs tone frequency) | `BUZZER_USE_TONE` | bool / Hz | **TBD** | TBD | TBD | TBD | TBD | Pending |

---

## 8. Session log (narrative)

Use this section for free-form notes per calibration session.

### Session template

```text
Session ID:
Date:
Tester:
Hardware revision:
Weather / environment:
Bin liner / load state:
Summary:
Follow-up actions:
```

### Sessions

| Session ID | Date | Tester | Summary |
|------------|------|--------|---------|
| — | TBD | Steven | No calibration session recorded yet. |

---

## Related documents

- [`HARDWARE_GPIO_MAP.md`](HARDWARE_GPIO_MAP.md)
- [`../README.md`](../README.md)
