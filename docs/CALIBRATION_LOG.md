# Calibration log — Smart Dustbin (ESP8266)

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Product:** Simplified approach + lid + white LED (WP-11.1+)  
**Purpose:** Record **physical measurements** and acceptance for firmware constants in `SmartDustbin/config.h`.  
**Rule:** Do **not** invent **Accepted** values. Use measured data and session notes. **Provisional** firmware defaults are listed separately — they are **not** physical acceptance.

---

## Historical note (WP-11.1)

The following were **removed** from the product and are **out of scope** for this log’s active workflow:

- Waste-level HC-SR04 (D5/D6)
- Fill percentage and warning hysteresis
- Yellow LED and buzzer alerting

See [`WP-11.1_COMPLETION_REPORT.md`](WP-11.1_COMPLETION_REPORT.md). Older sections that described those parameters are **not** maintained here.

---

## How to use this log

1. Measure on the **assembled bin** under realistic conditions.
2. For each parameter, record **Test date**, **Test conditions**, **Measured value**, **Observations**, and **Acceptance** (`Pending` / `Accepted` / `Rejected`).
3. When a value is **Accepted**, update firmware `config.h` in a dedicated work package and cite the commit in **Observations**.
4. Do **not** mark **Accepted** for Steven’s **provisional** defaults until a session row documents the measurement.

---

## Firmware defaults vs acceptance

Values below in the **“Firmware default (provisional)”** column come from `config.h` at baseline `84b7ce9`. They are **development defaults**, **not** calibrated or accepted unless a row below is explicitly **Accepted**.

| Parameter | `config.h` symbol | Firmware default (provisional) | Physical acceptance |
|-----------|-------------------|----------------------------------|---------------------|
| Approach detect distance | `CONFIG_APPROACH_DETECT_CM` | **50** cm | **Pending** — **PROVISIONAL** |
| Approach clear distance | `CONFIG_APPROACH_CLEAR_CM` | **55** cm | **Pending** — **PROVISIONAL** |
| Lid closed angle | `CONFIG_SERVO_ANGLE_CLOSED` | **30**° | **Pending** — **PROVISIONAL** |
| Lid open angle | `CONFIG_SERVO_ANGLE_OPEN` | **120**° | **Pending** — **PROVISIONAL** |
| Close delay after clear | `CONFIG_LID_CLOSE_DELAY_MS` | **2000** ms | **Pending** |
| Approach sample period | `CONFIG_APPROACH_SAMPLE_MS` | **100** ms | **Pending** |
| Quiet after pulse | `CONFIG_ULTRASONIC_QUIET_MS` | **60** ms | **Pending** |

**Steven confirmation (WP-11.6):** Approach **50 / 55 cm** and servo **30° / 120°** remain **PROVISIONAL** — do not document as measured or accepted.

---

## 1. Approach sensor

| Parameter | Unit | Firmware default (provisional) | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------|----------------------------------|-----------|-----------------|----------------|--------------|------------|
| Detect (latch near) | cm | 50 (`CONFIG_APPROACH_DETECT_CM`) | TBD | TBD | TBD | TBD | **Pending** |
| Clear (latch far) | cm | 55 (`CONFIG_APPROACH_CLEAR_CM`) | TBD | TBD | TBD | TBD | **Pending** |
| Sample period | ms | 100 (`CONFIG_APPROACH_SAMPLE_MS`) | TBD | TBD | TBD | TBD | **Pending** |
| Pulse timeout | µs | 30000 (`CONFIG_APPROACH_PULSE_TIMEOUT_US`) | TBD | TBD | TBD | TBD | **Pending** |
| Valid range min / max | cm | 2 / 400 (`CONFIG_APPROACH_MIN_VALID_CM` / `MAX`) | TBD | TBD | TBD | TBD | **Pending** |
| Post-pulse quiet | ms | 60 (`CONFIG_ULTRASONIC_QUIET_MS`) | TBD | TBD | TBD | TBD | **Pending** |

---

## 2. Servo — lid angles

| Parameter | Unit | Firmware default (provisional) | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------|----------------------------------|-----------|-----------------|----------------|--------------|------------|
| Closed angle | degrees | 30 (`CONFIG_SERVO_ANGLE_CLOSED`) | TBD | TBD | TBD | TBD | **Pending** — **PROVISIONAL** |
| Open angle | degrees | 120 (`CONFIG_SERVO_ANGLE_OPEN`) | TBD | TBD | TBD | TBD | **Pending** — **PROVISIONAL** |

---

## 3. Lid close delay

| Parameter | Unit | Firmware default (provisional) | Test date | Test conditions | Measured value | Observations | Acceptance |
|-----------|------|----------------------------------|-----------|-----------------|----------------|--------------|------------|
| Delay after approach clears | ms | 2000 (`CONFIG_LID_CLOSE_DELAY_MS`) | TBD | TBD | TBD | TBD | **Pending** |

---

## 4. Session log (narrative)

### Session template

```text
Session ID:
Date:
Tester:
Hardware revision:
Environment:
Summary:
Follow-up actions:
```

### Sessions

| Session ID | Date | Tester | Summary |
|------------|------|--------|---------|
| — | TBD | Steven | No calibration session recorded as **Accepted** yet. Firmware uses provisional defaults above. |

---

## Related documents

- [`HARDWARE_GPIO_MAP.md`](HARDWARE_GPIO_MAP.md)
- [`../README.md`](../README.md)
- [`WP-11.6_DOCUMENTATION_AUDIT.md`](WP-11.6_DOCUMENTATION_AUDIT.md)