# WP-4 completion report — Waste fill-level intelligence

**Case ID:** CASE-20261112-SMARTDUSTBIN-END-TO-END  
**Work package:** WP-4  
**Date:** 2026-11-12  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-4 adds **`waste_fill_level`** to convert **valid** waste HC-SR04 distances into **fill percentage (0–100)** using configurable empty/full reference distances in `config.h`.

Invalid waste samples do **not** clear or overwrite the last valid fill percentage. Calibration validity (`empty_ref > full_ref`) is checked at boot; invalid references block percentage updates and are reported on Serial.

WP-3 scheduler, approach 50/55 cm behaviour, `[approach]` / `[waste]` lines, and **5 s heartbeat** are preserved (heartbeat extended with fill fields).

**Excluded:** servo, LEDs, buzzer, Wi-Fi, HTTP, dashboard, warning thresholds (WP-4+).

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/waste_fill_level.h` | API |
| `SmartDustbin/waste_fill_level.cpp` | Calculation + retained state |
| `docs/WP-4_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/config.h` | `CONFIG_WASTE_EMPTY_REF_CM`, `CONFIG_WASTE_FULL_REF_CM` (provisional) |
| `SmartDustbin/SmartDustbin.ino` | Apply fill on waste sample; enriched `[waste]` + heartbeat |

### Not modified

| Item |
|------|
| `ultrasonic_scheduler.*`, `waste_sensor.*` measurement path |
| `approach_sensor.*`, `pins.h`, legacy sketch |
| Buzzer GPIO |

---

## 3. Fill-percentage calculation and calibration configuration

**Formula (valid measurement, valid calibration):**

```text
fill_pct = (empty_ref_cm - measured_cm) / (empty_ref_cm - full_ref_cm) × 100
```

Result clamped to **0–100**. Smaller measured distance ⇒ fuller bin.

**Provisional defaults (NOT physically calibrated):**

| Symbol | Default | Note |
|--------|---------|------|
| `CONFIG_WASTE_EMPTY_REF_CM` | **40** | Replace after bench mount |
| `CONFIG_WASTE_FULL_REF_CM` | **10** | Replace after bench mount |

**Validation:** `empty_ref > full_ref` required. If false, `wasteFillLevelIsCalibrationValid()` is false for the session; no fill percentages are computed (FR-03).

---

## 4. Invalid-reading handling

| Condition | Fill % state |
|-----------|----------------|
| Timeout / out-of-range / invalid sample | **Unchanged** — last valid % retained |
| Valid sample + valid calibration | Recomputed and stored |
| Valid sample + **invalid** calibration | **Not computed**; Serial shows `cal_error=empty_ref_le_full_ref` |
| Before first valid sample | `fill_pct=none`, `fill_ever_valid=0` |

`wasteFillLevelHasValidPercent()` tracks whether any valid % was computed since boot (FR-04).

---

## 5. Serial diagnostics (FR-05)

**`[waste]`** (after each waste measurement):  
`measurement_valid`, distance or reason, `cal_valid`, optional `cal_error`, `fill_pct`, `fill_ever_valid`, `waste_last_valid_cm`.

**`[heartbeat]`** (5 s): existing approach fields + `waste_last_valid_cm`, `cal_valid`, `fill_pct`, `fill_ever_valid`.

**Boot banner:** WP-4, provisional refs with explicit “not physically calibrated”.

---

## 6. Compilation command and result

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Item | Value |
|------|--------|
| Exit code | **0** |
| Core | esp8266:esp8266 3.1.2 |

Upload / physical test: **not performed** (per assignment).

---

## 7. Known limitations

| ID | Limitation |
|----|------------|
| L-1 | Provisional 40/10 cm refs may produce meaningless % until calibrated on the real bin |
| L-2 | Integer division rounding on fill % |
| L-3 | No warning threshold / hysteresis (later WP) |
| L-4 | Fill updated only on valid **current** sample; held across invalid streaks |

---

## 8. Excluded functionality confirmation

Not implemented: servo/lid, LEDs, buzzer, Wi-Fi, HTTP, dashboard, persistence, additional sensors, scheduler rewrite.

---

## 9. Skill selection

**Path:** `atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not available**.

**Selected skill:** None.

---

**STOP:** Await Steven’s review and Craig’s assessment. Do not proceed to WP-5.

*End of WP-4 completion report.*
