# CASE-20261119-DUSTBIN-LED-D0 — White LED / GPIO16 Investigation

> **Superseded for current operation (WP-11.2+):** This investigation analysed **fill-driven** white/yellow LED behaviour that no longer exists in firmware. The current product drives the white LED on **D0** from **lid state** (ON while the lid is open). Retain this file for historical traceability only; use [`WP-11.2_COMPLETION_REPORT.md`](WP-11.2_COMPLETION_REPORT.md) and [`HARDWARE_GPIO_MAP.md`](HARDWARE_GPIO_MAP.md) for the active product.

**Case ID:** CASE-20261119-DUSTBIN-LED-D0  
**Repository:** SMART-DUSTBIN-ESP8266MOD  
**Mode:** Read-only firmware audit (no source changes, no upload)  
**Date:** 2026-11-19  
**Reviewer gate:** Await Craig and Steven before proposing or implementing changes.

---

## 1. Executive finding

Firmware **consistently assigns the white LED to NodeMCU D0 (ESP8266 GPIO16)** and drives it with the **same `pinMode` / `digitalWrite` pattern as the yellow LED on D3 (GPIO0)**. There is **no evidence of a wrong GPIO define, missing OUTPUT init, output inversion, or any other module writing GPIO16**.

The behaviour most consistent with the repo **without assuming defective hardware** is **application logic**, not a GPIO16 driver bug:

| Firmware state | White (D0 / GPIO16) | Yellow (D3 / GPIO0) |
|----------------|---------------------|---------------------|
| **UNKNOWN** (boot until first valid fill %, or invalid thresholds) | OFF (LOW) | OFF (LOW) |
| **NORMAL** (fill below warning latch) | ON (HIGH) | OFF (LOW) |
| **WARNING** (fill ≥ 80% latched) | OFF (LOW) | ON (HIGH) |

If the bin is in **WARNING** or **UNKNOWN**, the white output on D0 stays **LOW** by design. Moving the **same LED** to **D3** connects it to **GPIO0**, which may be **HIGH** in **WARNING**, so the LED can appear to “work on D3 only.” That matches observed bench behaviour without requiring a firmware pin mix-up.

**Classification:** **Primarily configuration / expected runtime state** (fill-driven gating), with **GPIO16 implementation in firmware appearing correct** for ESP8266 core 3.1.2. **Hardware** (D0 header, LED polarity, GPIO16 drive) remains **unverified on the failing unit** and should be ruled in or out using Serial plus a meter on D0 when `white=1`.

**Confidence (overall):** **Medium–high** for “no obvious firmware GPIO defect”; **medium** for “swap-to-D3 explained by WARNING/UNKNOWN”; **low** until Serial/`/api/status` corroborates runtime `white` vs physical voltage on D0.

---

## 2. Relevant source files and code references

| File | Role |
|------|------|
| `SmartDustbin/pins.h` | `PIN_LED_WHITE` → `D0` (GPIO16); `PIN_LED_YELLOW` → `D3` (GPIO0) |
| `SmartDustbin/gpio_init.cpp` | Sensors only; LEDs deferred to `led_status` |
| `SmartDustbin/led_status.cpp` | Pin init, fill hysteresis, `digitalWrite` for both LEDs |
| `SmartDustbin/led_status.h` | `LedFillIndicatorState`, getters |
| `SmartDustbin/config.h` | `CONFIG_WARN_FILL_PERCENT` (80), `CONFIG_WARN_CLEAR_PERCENT` (75) |
| `SmartDustbin/waste_fill_level.cpp` | Valid fill % prerequisite for NORMAL/WARNING |
| `SmartDustbin/SmartDustbin.ino` | `ledStatusBegin()` in `setup()`, `ledStatusUpdate()` each `loop()` |
| `SmartDustbin/application_status.cpp` | Snapshot `whiteLedOn` / `yellowLedOn` for HTTP |
| `docs/HARDWARE_GPIO_MAP.md` | Approved map: white D0/GPIO16, yellow D3/GPIO0, active-HIGH wiring |

---

## 3. GPIO16 / white LED — pin definition

```11:12:SmartDustbin/pins.h
#define PIN_LED_WHITE     D0  // GPIO16
#define PIN_LED_YELLOW    D3  // GPIO0
```

On FQBN `esp8266:esp8266:nodemcuv2`, core variant `nodemcu` defines **`static const uint8_t D0 = 16`**. `PIN_LED_WHITE` is therefore **GPIO16**, not a silkscreen/GPIO number mismatch. The project uses **NodeMCU `D0`/`D3` labels** that resolve to **numeric GPIO 16 and 0** via the board variant — consistent throughout `pins.h` and `led_status.cpp`.

---

## 4. GPIO16 initialization findings

**`gpio_init` does not touch D0/D3** (intentional after WP-6):

```16:20:SmartDustbin/gpio_init.cpp
  // Servo D7: initialised by lid_control (WP-5); leave pin untouched here.

  // D0/D3 LEDs: configured by led_status (WP-6).

  // D4 / GPIO2 buzzer: not configured (physically disconnected; boot strap)
```

**White LED init occurs only in `ledStatusBegin()`**, called from `setup()` after waste fill and lid modules:

```87:92:SmartDustbin/led_status.cpp
void ledStatusBegin() {
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);

  // After flash boot: indicators OFF (active HIGH wiring).
  setOutputs(false, false);
```

```122:132:SmartDustbin/SmartDustbin.ino
void setup() {
  Serial.begin(CONFIG_SERIAL_BAUD);
  delay(50);

  gpioInitSafeOutputsAndInputs();
  approachSensorBegin();
  wasteSensorBegin();
  wasteFillLevelBegin();
  ultrasonicSchedulerBegin();
  lidControlBegin();
  ledStatusBegin();
```

- **GPIO16 is configured as OUTPUT** before any intentional HIGH drive.
- **No accidental init of D0** elsewhere in the sketch (grep: only `led_status.cpp` uses `PIN_LED_WHITE`).

**ESP8266 core note (3.1.2):** `pinMode(16, OUTPUT)` and `digitalWrite(16, …)` use the separate **GP16** registers (`GP16E`, `GP16O`), not the GP0–GP15 bank. The firmware uses the standard Arduino API; the core implements GPIO16 explicitly. Firmware does **not** call `tone()`, PWM, or interrupts on D0 (unsupported / inappropriate on GPIO16).

**Board note:** NodeMCU variant sets `LED_BUILTIN_AUX` to **16**. Some modules expose an auxiliary LED on GPIO16; that does not change firmware pin assignment but may matter when interpreting “something on D0” visually.

---

## 5. White LED output control and state propagation

**Drive polarity:** Active **HIGH** when ON (not inverted in software):

```21:25:SmartDustbin/led_status.cpp
static void setOutputs(bool whiteOn, bool yellowOn) {
  digitalWrite(PIN_LED_WHITE, whiteOn ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellowOn ? HIGH : LOW);
  s_whiteOn = whiteOn;
  s_yellowOn = yellowOn;
```

**State → outputs:**

```72:81:SmartDustbin/led_status.cpp
  switch (st) {
    case LedFillIndicatorState::Unknown:
      setOutputs(false, false);
      break;
    case LedFillIndicatorState::Normal:
      setOutputs(true, false);
      break;
    case LedFillIndicatorState::Warning:
      setOutputs(false, true);
      break;
  }
```

**When white should be HIGH:** Only in **`LedFillIndicatorState::Normal`**, after:

1. Threshold config valid (`clear < warn`, 0–100).
2. At least one **valid** waste distance processed into fill % (`wasteFillLevelHasValidPercent()`).
3. Warning latch **not** set (fill &lt; 80% on first entry; hysteresis via `CONFIG_WARN_CLEAR_PERCENT` 75%).

```110:139:SmartDustbin/led_status.cpp
void ledStatusUpdate() {
  if (!s_thresholdsValid) {
    applyState(LedFillIndicatorState::Unknown, 0, false);
    return;
  }

  if (!wasteFillLevelHasValidPercent()) {
    applyState(LedFillIndicatorState::Unknown, 0, false);
    return;
  }
  // ...
  const LedFillIndicatorState next =
      s_warningLatched ? LedFillIndicatorState::Warning
                       : LedFillIndicatorState::Normal;

  applyState(next, fillPct, true);
}
```

**Main loop integration:**

```158:158:SmartDustbin/SmartDustbin.ino
  ledStatusUpdate();
```

Heartbeat and HTTP expose logical `white` / `yellow` vs fill (`ledStatusGetWhiteOn()`, `application_status`, `/api/status` JSON `leds.white_on`).

**Implication for the incident:** A white LED on **D0** stays dark during **UNKNOWN** and **WARNING**. If the waste sensor has not yet produced a valid reading, or fill is ≥ 80%, **D0 will not go HIGH** even though the LED works when moved to **D3** while **yellow** is driven HIGH.

---

## 6. Comparison with yellow LED (D3 / GPIO0)

| Aspect | White (`PIN_LED_WHITE` / D0 / GPIO16) | Yellow (`PIN_LED_YELLOW` / D3 / GPIO0) |
|--------|----------------------------------------|----------------------------------------|
| Definition | `pins.h` | `pins.h` |
| `pinMode(OUTPUT)` | `ledStatusBegin()` | Same |
| `digitalWrite` | `setOutputs()` | Same |
| Polarity | HIGH = ON | HIGH = ON |
| ON condition | NORMAL fill state | WARNING fill state |
| Other writers | None in repo | None in repo |
| Boot strap | No flash-boot conflict (per hardware doc) | GPIO0 must be HIGH at reset; firmware drives LOW after boot for “off” |

There is **no asymmetric bug** in the yellow path that would suggest white was wired differently in software. The **functional** asymmetry is **which fill state drives which pin**.

---

## 7. Other subsystems and GPIO16

Repository search shows **no** `digitalWrite`/`pinMode` on D0/GPIO16 outside `led_status.cpp`. Servo uses **D7 (GPIO13)** only. HC-SR04 TRIG pins are D1/D5. **Wi-Fi / HTTP** read LED state via getters; they do not manipulate GPIO16.

---

## 8. Root-cause assessment

| Hypothesis | Assessment | Confidence |
|------------|------------|------------|
| Wrong pin / GPIO number in firmware | **Ruled out** — `D0` → 16 on nodemcu | High |
| GPIO16 not initialized OUTPUT | **Ruled out** — `ledStatusBegin()` | High |
| Inverted white LED logic | **Ruled out** — active HIGH | High |
| Stray writes to GPIO16 | **Ruled out** — single module | High |
| White never driven because fill state is UNKNOWN or WARNING | **Plausible** — matches design | Medium–high |
| GPIO16 API misuse in firmware | **Unlikely** — standard core path, compile OK | Medium |
| D0 header / LED wiring / GPIO16 silicon or drive strength | **Possible** if Serial shows `white=1` but no ~3.3 V on D0 | Unverified (needs bench) |
| User expected “white on at boot” vs firmware “both off until valid fill” | **Plausible misunderstanding** | Medium |

**Primary conclusion:** Treat as **expected application behaviour** or **runtime state (fill / sensor validity)** first; **not** a clear firmware GPIO assignment defect. A **firmware change is not necessary** to “fix” pin mapping unless product policy changes (e.g. indictor on boot).

---

## 9. Recommended next diagnostic step

1. **Serial or dashboard at 115200 baud** — on a failing run, record every 5 s:
   - `[heartbeat] … led=… white=… yellow=… fill_pct=…`
   - Or `GET /api/status` → `leds.indicator`, `leds.white_on`, `leds.yellow_on`, `fill.last_valid_percent`.

2. **Interpretation table**

   | `led` | `white` | `yellow` | Expected D0 voltage (active HIGH) | Expected D3 |
   |-------|---------|----------|-------------------------------------|-------------|
   | UNKNOWN | 0 | 0 | ~0 V | ~0 V |
   | NORMAL | 1 | 0 | ~3.3 V | ~0 V |
   | WARNING | 0 | 1 | ~0 V | ~3.3 V |

3. If **`white=1` (NORMAL)** but D0 stays ~0 V with LED+resistor on approved net → **hardware**: continuity D0 header ↔ ESP8266 GPIO16, LED polarity, common GND.

4. If **`white=0`** and **`led=WARNING`** while testing “white” LED on D3 → **relocate is expected**; confirm fill % and provisional refs in `config.h` (`CONFIG_WASTE_EMPTY_REF_CM` / `CONFIG_WASTE_FULL_REF_CM`).

5. Optional **isolation test** (bench, after review): minimal sketch toggling only `D0` — only if step 3 implicates hardware with `white=1` asserted.

---

## 10. Is a firmware change necessary?

**No**, for correcting pin assignment or GPIO16 init as implemented today.

Consider firmware changes **only after review** if product requirements differ, for example:

- Visual “alive” indicator on boot independent of fill (policy).
- Explicit logging when stuck in UNKNOWN (sensor/calibration).
- Documented bench procedure for GPIO16 (not required for correctness given core support).

Do **not** swap white/yellow pins in firmware without hardware doc approval.

---

## 11. Build verification (baseline)

**Command (requested):**

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

**Environment note:** `arduino-cli` was not on `PATH` in the audit shell. Compile was run with the Arduino IDE bundled binary:

`%LOCALAPPDATA%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`

| Item | Result |
|------|--------|
| Exit code | **0 (success)** |
| ESP8266 core (installed) | **esp8266:esp8266 3.1.2** |
| RAM (global/static) | **28712 / 80192 bytes (35%)** — DATA 1496, RODATA 1048, BSS 26168 |
| IRAM | **61387 / 65536 bytes (93%)** |
| Flash (IROM) | **286040 / 1048576 bytes (27%)** |

Compile success confirms **no build-time pin/API errors**; it does not prove on-target GPIO16 timing without upload (out of scope).

---

## 12. Remains unverified (no on-device evidence in this audit)

- Actual runtime `led` / `white` / `yellow` / `fill_pct` on the incident unit.
- Whether the incident occurred in **UNKNOWN**, **NORMAL**, or **WARNING**.
- Multimeter measurement on **D0** when firmware reports `white=1`.
- Physical wiring (220 Ω, anode/cathode, shared GND) on **D0** vs **D3** header.
- Cold-boot / strap interaction for **yellow** on D3 (documented separately; not the white pin).
- Whether **provisional** waste empty/full refs cause high fill % and chronic **WARNING** (yellow on D3).

---

**End of report — no firmware modifications performed.**