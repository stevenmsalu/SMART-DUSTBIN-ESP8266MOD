# WP-5 completion report — Automatic lid control

**Case ID:** CASE-20261112-SMARTDUSTBIN-END-TO-END  
**Work package:** WP-5  
**Date:** 2026-11-12  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-5 adds **`lid_control`** (non-blocking **`millis()`** state machine + **`Servo`** on **D7 / GPIO13**) driven by **`approachSensorIsApproachDetected()`**:

- **Open** on approach active (cancel pending close).
- **Close** after approach clear + **`CONFIG_LID_CLOSE_DELAY_MS`** (re-armed only while lid is OPEN).
- No repeated `write()` when target angle unchanged.

WP-2/3 scheduler, WP-4 fill %, `[approach]` / `[waste]`, and **5 s heartbeat** retained (heartbeat adds lid fields).

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/lid_control.h` | Lid API |
| `SmartDustbin/lid_control.cpp` | Servo + state machine |
| `docs/WP-5_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/config.h` | Servo angles + close delay |
| `SmartDustbin/SmartDustbin.ino` | `lidControlBegin/Update`, banner, heartbeat |
| `SmartDustbin/gpio_init.cpp` | Servo pin owned by `lid_control` |

### Unchanged

Ultrasonic scheduler, approach/waste/fill modules, legacy sketch, buzzer, GPIO map.

---

## 3. Servo configuration (provisional)

| Symbol | Default | Note |
|--------|---------|------|
| `CONFIG_SERVO_ANGLE_CLOSED` | **30** | Not physically calibrated |
| `CONFIG_SERVO_ANGLE_OPEN` | **120** | Not physically calibrated |
| `CONFIG_LID_CLOSE_DELAY_MS` | **2000** | After approach clears |

Angles validated **0–180** at init; attach fails safe with Serial error if invalid.

**Power:** External **5 V** for servo; signal on **D7**; shared GND only (documented in hardware doc; not a firmware change).

---

## 4. Automatic opening and closing

| Event | Behaviour |
|-------|-----------|
| `approach_detected` **true** | Cancel `close_delay`; transition to **OPEN**; servo → open angle (once) |
| Already **OPEN** + approach | No extra servo commands |
| `approach_detected` **false** + lid **OPEN** | Start close delay (once) |
| Delay elapses, still clear | **CLOSED**; servo → closed angle |
| Approach during delay | `close_delay_cancelled`; remain **OPEN** |

Lid stays **OPEN** during close delay (FR-04).

---

## 5. Non-blocking state machine

- **`lidControlUpdate(now, approach)`** each `loop()` — no `delay()` for lid timing.
- Close delay: `(now - closeDelayStartMs) >= CONFIG_LID_CLOSE_DELAY_MS` (unsigned **millis** wrap-safe for normal intervals).
- Ultrasonic scheduler + heartbeat unchanged.

**States:** `LidPositionState::Closed | Open` plus internal **`closeDelayPending`** while OPEN and counting down.

---

## 6. Serial diagnostics

| Tag | When |
|-----|------|
| `[lid] init_closed` | Boot — closed angle |
| `[lid] opening` / `state=OPEN` | First transition to open |
| `[lid] close_delay_start` | Approach cleared |
| `[lid] close_delay_cancelled approach` | Re-approach during delay |
| `[lid] closing` / `state=CLOSED` | After delay |
| `[heartbeat] … lid=OPEN|CLOSED servo_target_deg=… close_pending=…` | Every 5 s |

Servo `write` only logged when angle **changes** (avoids spam).

---

## 7. Compilation

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Result |
|--------|
| Exit code **0** (esp8266:esp8266 3.1.2, **Servo** library via core) |

Upload / bench: **not performed**.

---

## 8. Known limitations

| ID | Note |
|----|------|
| L-1 | Provisional 30°/120° may not match mechanical lid |
| L-2 | No dwell timer while open beyond close-delay-on-clear (WP spec as given) |
| L-3 | `Servo.write` timing is immediate; no motion-complete feedback |
| L-4 | IRAM use ~93% with Servo — monitor if future features added |

---

## 9. Excluded functionality

Not implemented: LEDs, buzzer, Wi-Fi, HTTP, dashboard, remote lid, fill-dependent lid logic, scheduler rewrite.

---

## 10. Skill selection

`atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not available**. **No skill used.**

---

**STOP** — await Steven / Craig review. Do not start WP-6.

*End of WP-5 completion report.*
