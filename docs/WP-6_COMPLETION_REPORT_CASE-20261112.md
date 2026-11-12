# WP-6 completion report — LED status indicators

**Case ID:** CASE-20261112-SMARTDUSTBIN-END-TO-END  
**Work package:** WP-6  
**Date:** 2026-11-12  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-6 adds **`led_status`** to drive **white (D0 / GPIO16)** and **yellow (D3 / GPIO0)** from **WP-4** last valid fill % with **80% / 75%** hysteresis.

- **UNKNOWN:** both OFF until first valid fill %.
- **NORMAL:** white ON, yellow OFF.
- **WARNING:** yellow ON, white OFF.

Invalid waste samples do not change fill % (WP-4) → LED state **retained**. Serial `[led]` logs only on **state/output changes**; heartbeat summarizes LED fields.

Approach, scheduler, fill math, and lid control **unchanged**.

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/led_status.h` | API + `LedFillIndicatorState` |
| `SmartDustbin/led_status.cpp` | Hysteresis + GPIO |
| `docs/WP-6_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/config.h` | `CONFIG_WARN_FILL_PERCENT`, `CONFIG_WARN_CLEAR_PERCENT` |
| `SmartDustbin/gpio_init.cpp` | LEDs owned by `led_status` |
| `SmartDustbin/SmartDustbin.ino` | `ledStatusBegin/Update`, banner, heartbeat |

### Unchanged

`waste_fill_level.*`, scheduler, lid, buzzer, GPIO map, legacy sketch.

---

## 3. GPIO assignments

| LED | Pin | GPIO | Drive |
|-----|-----|------|--------|
| White | D0 | 16 | Active HIGH (via `pins.h`) |
| Yellow | D3 | 0 | Active HIGH |

Assignments remain in **`pins.h`** only.

---

## 4. Warning and clearing thresholds

| Symbol | Default | Rule |
|--------|---------|------|
| `CONFIG_WARN_FILL_PERCENT` | **80** | Enter warning when fill ≥ value |
| `CONFIG_WARN_CLEAR_PERCENT` | **75** | Leave warning when fill ≤ value |

Boot validation: both **0–100**, **clear < warn**; otherwise thresholds invalid → UNKNOWN, both OFF, Serial error.

---

## 5. Hysteresis implementation

Internal latch `s_warningLatched`:

- **false → true** when `fill_pct >= warn`
- **true → false** when `fill_pct <= clear`
- Between **75–80** while latched: stay **WARNING**; while not latched: stay **NORMAL**

Uses **last valid** fill only (`wasteFillLevelGetLastValidPercent`).

---

## 6. Invalid-reading behaviour

| Situation | LEDs |
|-----------|------|
| No valid fill % yet | UNKNOWN, both OFF |
| Invalid waste reading (fill % unchanged) | Previous NORMAL/WARNING **unchanged** |
| Invalid does not imply empty | No forced NORMAL |

---

## 7. LED diagnostics

**On transition:** `[led] state=NORMAL|WARNING|UNKNOWN white=… yellow=… warn_pct=… clear_pct=… fill_pct=…`

**Boot (once):** threshold banner + GPIO0 bench note.

**Heartbeat (5 s):** `led=… white=… yellow=…` (no spam between changes).

---

## 8. GPIO0 boot considerations

- Yellow LED on **GPIO0**; flash boot needs **GPIO0 HIGH** at reset.
- Firmware configures **OUTPUT LOW** (LED off) **after** `setup()` begins — does not alter reset strap window.
- **Low at runtime** turns LED off but loads GPIO0 — Steven must run **cold-boot** tests with yellow LED wired (HV-1 in hardware doc).
- No wiring or circuit changes in WP-6.

---

## 9. Compilation

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

**Exit code 0** (esp8266:esp8266 3.1.2).

---

## 10. Known limitations

| ID | Note |
|----|------|
| L-1 | Thresholds provisional until calibration |
| L-2 | LED state updates each loop but GPIO writes only on change |
| L-3 | GPIO0 electrical load at power-on not verified in firmware |

---

## 11. Excluded functionality

Not implemented: buzzer, Wi-Fi, HTTP, dashboard, servo/scheduler/fill changes, extra sensors.

---

## 12. Skill selection

`atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not available**. **No skill used.**

---

**STOP** — await Steven / Craig review. Do not start WP-7.

*End of WP-6 completion report.*
