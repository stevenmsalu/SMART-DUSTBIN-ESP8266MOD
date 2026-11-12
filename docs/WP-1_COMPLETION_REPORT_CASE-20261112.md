# WP-1 completion report — Firmware foundation and safe boot skeleton

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Work package:** WP-1  
**Date:** 2026-11-12  
**Agent:** Cursor (Implementation Agent)  
**Authority:** Steven (approved WP-0; WP-1 authorised)  
**Mode:** Controlled implementation — firmware foundation only

---

## 1. Summary of implementation

WP-1 added the **production Arduino sketch** `SmartDustbin/` with central **`pins.h`**, provisional **`config.h`**, **`gpio_init`** module for safe idle GPIO states, and a minimal **`SmartDustbin.ino`** that initialises hardware and prints a **5 s Serial heartbeat**.

No HC-SR04 logic, servo motion, LEDs policy, buzzer alerts, Wi-Fi, HTTP, or dashboard code was added.

The legacy **`smart-dustbin-programme/`** buzzer diagnostic sketch was **preserved unchanged**.

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/SmartDustbin.ino` | Entry point: Serial, GPIO init, heartbeat |
| `SmartDustbin/pins.h` | Approved pin map |
| `SmartDustbin/config.h` | Baud, heartbeat; TBD placeholders as comments |
| `SmartDustbin/gpio_init.h` | GPIO init API |
| `SmartDustbin/gpio_init.cpp` | Safe idle pin modes and levels |
| `SmartDustbin/wifi_config.h.example` | Template for local `wifi_config.h` (future WP) |
| `docs/WP-1_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `README.md` | WP-1 status, `SmartDustbin/` build path, layout tree, doc link |

### Deliberately untouched

| File | Reason |
|------|--------|
| `smart-dustbin-programme/smart-dustbin-programme.ino` | Assignment: preserve diagnostic |
| `.gitignore` | Already covers `wifi_config.h` |
| Audit reports, hardware/calibration docs (except README cross-links) | WP-1 scope |
| Empty `sensors/`, `actuators/`, `services/`, `dashboard/` | Not created — no empty modules |

---

## 3. Final firmware structure

```text
SmartDustbin/
├── SmartDustbin.ino
├── config.h
├── pins.h
├── gpio_init.h
├── gpio_init.cpp
└── wifi_config.h.example    → copy to wifi_config.h (gitignored, WP-7+)
```

**Not created (later WPs):** `sensors/`, `actuators/`, `services/`, `dashboard/`.

**Parallel sketch (legacy):** `smart-dustbin-programme/` unchanged.

---

## 4. GPIO initialization and startup sequence

**Order in `setup()`:**

1. `Serial.begin(115200)` (`CONFIG_SERIAL_BAUD`).
2. Short `delay(50)` for USB-serial settle (no long blocking after init).
3. `gpioInitSafeOutputsAndInputs()` — see table below.
4. Boot banner and status lines on Serial.
5. `loop()`: non-blocking heartbeat every `CONFIG_HEARTBEAT_MS` (5000 ms).

| Pin | Label | GPIO | Mode | Idle level | Purpose in WP-1 |
|-----|-------|------|------|------------|-----------------|
| Approach TRIG | D1 | 5 | OUTPUT | LOW | HC-SR04 inactive |
| Approach ECHO | D2 | 4 | INPUT | — | No reads |
| Waste TRIG | D5 | 14 | OUTPUT | LOW | HC-SR04 inactive |
| Waste ECHO | D6 | 12 | INPUT | — | No reads |
| Servo | D7 | 13 | OUTPUT | LOW | No PWM / no Servo lib |
| White LED | D0 | 16 | OUTPUT | LOW | Off |
| Yellow LED | D3 | 0 | OUTPUT | LOW | Off |
| Buzzer | D4 | 2 | OUTPUT | LOW | Off (active HIGH) |

**D8 / GPIO15:** Not used; NodeMCU boards typically bias GPIO15 LOW for boot — consistent with ESP8266 flash-boot requirement.

---

## 5. Boot-strapping considerations

### ESP8266 requirements (flash boot)

| GPIO | Node | Required at reset | WP-1 firmware after `setup()` |
|------|------|-------------------|-------------------------------|
| GPIO0 | D3 | HIGH | OUTPUT **LOW** (yellow LED off) |
| GPIO2 | D4 | HIGH | OUTPUT **LOW** (buzzer off) |
| GPIO15 | D8 | LOW | Unconfigured (board default) |

### Wiring vs boot (engineering analysis — not physically verified)

**Assumed LED wiring (per hardware doc):** GPIO → 220 Ω → LED anode → cathode → GND (**active HIGH = ON**).

| Circuit | Boot-time concern | Runtime (WP-1) | Limitation |
|---------|-------------------|------------------|------------|
| **Yellow LED on GPIO0** | Flash boot needs GPIO0 **HIGH** during reset. Before `pinMode`, pin is input; chip **internal pull-up** tends to HIGH if the LED chain does not load the pin LOW. | Firmware drives **LOW** to keep LED off → intentional for operation, not for reset window. | If the LED/resistor presents a strong pull-low at power-on, boot can fail. **Steven must cold-boot test with yellow circuit connected (HV-1).** |
| **Buzzer on GPIO2** | Reset requires GPIO2 **HIGH**; ESP8266 **internal pull-up** on GPIO2. Active buzzer between GPIO2 and GND may draw current during reset if it turns on briefly. | **LOW** = buzzer off. | Unknown buzzer current/type may stress GPIO or audibly chirp at power-on. **Steven must cold-boot test with buzzer connected (HV-2).** |
| **White LED on GPIO16** | No flash-boot strap conflict. | LOW = off. | Lower risk. |
| **Servo on GPIO13** | No strap conflict. Static LOW is not valid servo PWM; mechanical hold is **undefined** until WP-5 — acceptable for WP-1. | No movement commanded. | Servo may hold last position or jitter slightly — observe on bench. |

**Important:** Successful **compilation does not prove** boot safety. Only repeated power-cycle tests with full wiring prove it.

---

## 6. Compilation results and evidence

| Item | Result |
|------|--------|
| Toolchain | Arduino IDE bundled `arduino-cli` **1.5.1** |
| ESP8266 core | **esp8266:esp8266 3.1.2** installed |
| FQBN | `esp8266:esp8266:nodemcuv2` (NodeMCU 1.0 / ESP-12E class) |
| Command | `arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin` |
| Exit code | **0 (success)** |
| Flash / RAM (build output) | IROM ~237200 B; static RAM use ~28172 B reported by core |

No external libraries beyond the **ESP8266 Arduino Core**. No `WiFi`, `ESP8266WiFi`, or Servo linkage in this sketch.

**Not performed:** Upload to COM5 (assignment: Steven performs physical testing).

**Steven — recommended:** Re-verify compile in Arduino IDE GUI with **NodeMCU 1.0 (ESP-12E Module)** selected, then flash and run §10 tests.

---

## 7. Known limitations and unresolved hardware risks

| ID | Limitation |
|----|------------|
| L-1 | Compile succeeded via CLI only; bench upload not performed by agent |
| L-2 | GPIO0/GPIO2 idle LOW after setup vs strap pins — hardware-dependent |
| L-3 | Servo signal held LOW without PWM — lid position not guaranteed |
| L-4 | ECHO pins as INPUT with no pull — acceptable until ranging starts |
| L-5 | All calibration constants remain **TBD** in `config.h` (comments only) |
| L-6 | `wifi_config.h` not required for WP-1 build; example file only |

---

## 8. Deviations from approved architecture

| Topic | Approved | WP-1 actual | Rationale |
|-------|----------|-------------|-----------|
| Sketch folder name | Roadmap showed `smart-dustbin-programme/` | **`SmartDustbin/`** per WP-1 assignment §3 | Explicit assignment path |
| Wi-Fi file name | `wifi_config.h` + `secrets.h` in README | **`wifi_config.h.example`**; `wifi_config.h` gitignored | Matches assignment; `.gitignore` already lists `wifi_config.h` |
| Subdirs `sensors/`, etc. | Long-term layout | **Omitted** until code exists | Assignment: no empty modules |

No pin assignment changes.

---

## 9. WP-1 acceptance checklist

| # | Criterion | Met? |
|---|-----------|------|
| 1 | Central `pins.h` with approved map | Yes |
| 2 | `config.h` without invented calibration | Yes |
| 3 | Safe GPIO init (TRIG LOW, outputs idle, no automation) | Yes |
| 4 | Boot strap pins considered and documented | Yes |
| 5 | Main `.ino` concise; logic in modules | Yes |
| 6 | No WP-2+ features | Yes |
| 7 | No PlatformIO / ESP32 | Yes |
| 8 | Diagnostic sketch preserved | Yes |
| 9 | No commit/push | Yes |
| 10 | Completion report + physical test list | Yes |
| 11 | Compilation evidence | Yes — `arduino-cli` exit 0 |

---

## 10. Recommended physical tests for Steven

**Do not upload claimed successful compile until local compile passes.**

| Test | Procedure | Pass criteria |
|------|-----------|---------------|
| T-1 Local compile | Arduino IDE → `SmartDustbin.ino` → Verify/Upload build only | Zero errors |
| T-2 Serial boot | 115200 baud after flash | Banner + `[boot] GPIO safe idle` + heartbeat every 5 s |
| T-3 Actuators idle | Visual/listen | LEDs off, buzzer silent, lid unmoved by firmware |
| T-4 Cold boot ×10 | Full wiring, power cycle | Enters flash mode every time; Serial banner appears |
| T-5 GPIO0 / yellow | During/after boot | Note any LED flash at power-on; document in calibration/hardware log |
| T-6 GPIO2 / buzzer | Power-on | No boot loop; note any chirp at reset |
| T-7 Servo line | Scope or ear on servo | No sustained PWM; no unexpected slew |
| T-8 TRIG lines | Optional DMM/logic probe later | Remains LOW (WP-2 will add pulses) |

Record results in a future WP completion report or `docs/CALIBRATION_LOG.md` session table.

---

## 11. Skill selected

**Path:** `atlas-docs-project-source/Workspace Meta/.cursor.skills/`  
**Selected skill:** **None** — path not present in workspace.

**Rationale:** Executed per WP-1 assignment and WP-0 hardware baseline.

---

## 12. Recommendation for WP-2

After WP-1 approval: HC-SR04 driver for approach sensor only, Serial debug distances, still non-blocking foundation — per audit work packages.

---

*End of WP-1 completion report — await Craig review and Steven approval before WP-2.*
