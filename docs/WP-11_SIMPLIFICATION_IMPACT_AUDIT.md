# WP-11 — Simplification Impact Audit (Read-Only)

**Case ID:** CASE-20261119-DUSTBIN-CALIBRATION-01 (pipeline) / **WP-11**  
**Repository:** SMART-DUSTBIN-ESP8266MOD  
**Audit mode:** Read-only — no source, config, or git changes during audit  
**Source of truth:** Committed `main` at audit time  
**Audience:** Steven and Craig — review before implementation  
**Date:** 2026-11-19  

---

## 1. Executive summary

The current firmware is a **full-stack monitor**: approach HC-SR04, coordinated dual-sensor scheduling, waste HC-SR04, fill percentage, fill-driven **white/yellow** LEDs, lid servo with close delay, Wi-Fi, HTTP JSON API, and embedded dashboard.

The approved **simplified product** is an **approach-and-open dustbin**: one ultrasonic sensor, lid automation (existing delay + re-approach cancel), and **white LED tied to “lid open” semantics** — not fill level.

**Critical gap:** Today **`led_status` does not follow approach or lid state**. White LED is driven only by **fill % hysteresis** (`LedFillIndicatorState::Normal`). Removing waste sensing without **redesigning LED control** would leave the white LED **off indefinitely** (UNKNOWN state). Implementation must **repurpose or replace** `led_status` to mirror `lid_control` open/closed behaviour (see §5).

**Scheduler impact:** Waste removal allows **dropping dual-sensor arbitration** in `ultrasonic_scheduler`; approach-only timing reduces acoustic quiet gaps and simplifies the main loop. **No shared resource** ties waste removal to servo timing beyond current loop ordering.

---

## 2. Git status (audit start)

```
On branch main
Your branch is up to date with 'origin/main'.
nothing to commit, working tree clean
```

Checkpoint commit confirmed — safe baseline for WP-11 implementation branching.

---

## 3. Current architecture summary

### 3.1 Runtime data flow (as committed)

```text
                    ┌─────────────────────────────────────────────────────────┐
                    │                      SmartDustbin.ino                   │
                    │  setup: gpio_init → approach → waste → fill → scheduler │
                    │         → lid → led_status → wifi → http                 │
                    └─────────────────────────────────────────────────────────┘
                      loop order (each iteration):
                      1. lidControlUpdate(now, approachSensorIsApproachDetected())
                      2. ultrasonicSchedulerTick(now) → Approach | Waste | None
                      3. on Approach: printApproachReading(...)
                      4. on Waste: wasteFillLevelApplyReading(...); printWaste(...)
                      5. ledStatusUpdate()          ← reads waste_fill_level only
                      6. wifiManagerUpdate / httpApiHandleClient
                      7. heartbeat / applicationStatus (5 s)
```

### 3.2 Module responsibilities

| Module | Responsibility |
|--------|----------------|
| `gpio_init` | Idle TRIG LOW; ECHO INPUT for **both** HC-SR04 pairs |
| `hcsr04_common` | Shared `pulseIn` ranging for any TRIG/ECHO |
| `approach_sensor` | D1/D2 ranging; hysteresis detect/clear (50/55 cm) |
| `waste_sensor` | D5/D6 ranging; last-valid distance cache |
| `waste_fill_level` | Empty/full refs → fill % (clamped 0–100) |
| `ultrasonic_scheduler` | One shot per tick; alternates approach/waste when both due; `CONFIG_ULTRASONIC_QUIET_MS` guard |
| `lid_control` | Servo on D7; open on approach; close delay; re-approach cancels delay |
| `led_status` | D0 white + D3 yellow; fill thresholds 80%/75% |
| `application_status` | Snapshot for Serial `[app]` and HTTP |
| `wifi_manager` | STA connect / reconnect |
| `http_api` + `dashboard_page` | `/`, `/api/status`, `/api/health` |
| `config.h` / `pins.h` | Central constants and GPIO map |

### 3.3 Approved target behaviour vs firmware today

| # | Target behaviour | Current implementation | Match? |
|---|------------------|------------------------|--------|
| 1 | Approach detects person | `approach_sensor` + scheduler | **Yes** |
| 2 | Servo opens lid | `lid_control` on `approachDetected` | **Yes** |
| 3 | White LED ON on approach | White ON only in fill **NORMAL** after valid waste % | **No** |
| 4 | Close delay when person leaves | `CONFIG_LID_CLOSE_DELAY_MS` while `!approachDetected` and lid was open | **Yes** |
| 5 | Delay expires → close lid + white OFF | Close yes; white not tied to lid | **Partial** |
| 6 | Re-approach before close → cancel delay, stay open | `close_delay_cancelled approach` in `lid_control` | **Yes** |

**White LED during close delay:** `lid_control` keeps `s_lidState == Open` until the delay elapses, so mapping **white = (lid state == Open)** satisfies “LED on while open, including countdown” without extra logic.

---

## 4. Execution path trace (approach → lid → LED)

### 4.1 Approach detection

1. `ultrasonicSchedulerTick` fires `approachSensorRunMeasurement()` when due (~100 ms).
2. `hcsr04Measure(PIN_APPROACH_TRIG, PIN_APPROACH_ECHO, …)` blocks up to timeout.
3. `updateApproachState`: distance ≤ `CONFIG_APPROACH_DETECT_CM` → detected; ≥ `CONFIG_APPROACH_CLEAR_CM` → cleared (hysteresis band).

### 4.2 Lid control (independent of fill/LED)

`lidControlUpdate` uses **current** `approachSensorIsApproachDetected()`:

- **Approach true:** cancel pending close delay; if closed → open servo.
- **Approach false且 lid open:** start delay once; after `CONFIG_LID_CLOSE_DELAY_MS` → close servo.

No calls to `waste_*` or `led_status`.

### 4.3 LED control (today — to be replaced)

`ledStatusUpdate` → `wasteFillLevelHasValidPercent()` / `wasteFillLevelGetLastValidPercent()` → warning latch → `digitalWrite(PIN_LED_WHITE|YELLOW)`.

**No coupling** to `lidControlGetLidState()` or `approachSensorIsApproachDetected()`.

### 4.4 Loop ordering note (regression-sensitive)

`lidControlUpdate` runs **before** the scheduler may refresh approach state in the same iteration. Lid therefore reacts to the **previous** cycle’s detection (one sample latency). This predates WP-11 and should be **preserved intentionally** unless a deliberate latency improvement is approved.

---

## 5. Functionality to retain

| Area | Files / symbols | Notes |
|------|-----------------|-------|
| Board / toolchain | NodeMCU 1.0 ESP-12E, `SmartDustbin/` layout | Unchanged |
| Approach sensor | `approach_sensor.*`, `hcsr04_common.*` | Keep; may call without scheduler wrapper |
| Approach config | `CONFIG_APPROACH_*`, `CONFIG_HCSR04_US_PER_CM` | Retain |
| Lid / servo | `lid_control.*`, `CONFIG_SERVO_*`, `CONFIG_LID_CLOSE_DELAY_MS` | Retain logic as-is |
| White LED GPIO | `PIN_LED_WHITE` (D0 / GPIO16) | Retain pin; **change driver policy** |
| Wi-Fi | `wifi_manager.*`, `wifi_config.h` (gitignored pattern) | Retain |
| HTTP server | `http_api.*`, routes `/`, `/api/status`, `/api/health` | Retain; trim JSON |
| Dashboard shell | `dashboard_page.*` | Retain; remove fill/waste UI |
| Serial diagnostics | `[approach]`, `[lid]`, heartbeat (trimmed) | Retain relevant fields |
| Quiet period after TRIG | `CONFIG_ULTRASONIC_QUIET_MS` | Optional but recommended if keeping shared `hcsr04Measure` |

---

## 6. Functionality to remove

### 6.1 Firmware modules (candidates for deletion)

| Module | Reason |
|--------|--------|
| `waste_sensor.cpp` / `waste_sensor.h` | Scope: no internal waste HC-SR04 |
| `waste_fill_level.cpp` / `waste_fill_level.h` | Scope: no fill % |
| `ultrasonic_scheduler.cpp` / `.h` | **Optional delete** — dual-sensor coordination unnecessary; replace with approach-only timer in `SmartDustbin.ino` or thin `approach_scheduler` |

### 6.2 Configuration (`config.h`)

Remove or stop referencing:

- `CONFIG_WASTE_SAMPLE_MS`, `CONFIG_WASTE_PULSE_TIMEOUT_US`, `CONFIG_WASTE_MIN_VALID_CM`, `CONFIG_WASTE_MAX_VALID_CM`
- `CONFIG_WASTE_EMPTY_REF_CM`, `CONFIG_WASTE_FULL_REF_CM`
- `CONFIG_WARN_FILL_PERCENT`, `CONFIG_WARN_CLEAR_PERCENT`

Retain `CONFIG_ULTRASONIC_QUIET_MS` if approach measurements still use post-pulse quiet time.

### 6.3 GPIO (`pins.h` / `gpio_init.cpp`)

| Symbol | WP-11 action |
|--------|----------------|
| `PIN_WASTE_TRIG` (D5), `PIN_WASTE_ECHO` (D6) | Remove from map; remove init in `gpio_init` |
| `PIN_LED_YELLOW` (D3 / GPIO0) | Remove from firmware driver; **do not** configure as OUTPUT LOW for “yellow off” (reduces GPIO0 boot-load concern if pin unwired) |
| `PIN_BUZZER` (D4) | Already unused in production sketch; remains out of scope unless later WP |

Hardware may still have a yellow LED physically on D3 — firmware should leave pin **unconfigured** or documented INPUT per hardware sign-off.

### 6.4 `led_status` — transform, not just delete

Remove:

- `LedFillIndicatorState`, warning/normal/unknown fill semantics
- Yellow output, threshold validation, `waste_fill_level` include
- GPIO0 boot banner in `ledStatusBegin`

Implement (names may change in implementation):

- `ledStatusBegin()`: `pinMode(PIN_LED_WHITE, OUTPUT)`; white OFF at boot
- `ledStatusUpdate()`: set white HIGH iff `lidControlGetLidState() == Open` (edge-detect logging optional)
- Getters: `ledStatusGetWhiteOn()`; drop yellow/indicator enum or map indicator to `OPEN`/`CLOSED` for API compatibility

Alternative: fold white GPIO into `lid_control` — **not recommended** without team preference (mixes actuator + indicator; harder to test in isolation).

### 6.5 `SmartDustbin.ino`

Remove includes, setup calls, loop branches, banner lines, heartbeat fields, and printers for waste/fill/warning/yellow.

Add: call simplified approach timing + updated `ledStatusUpdate`.

### 6.6 `application_status.*`

Remove snapshot fields: all `waste*`, `fill*`, `ledIndicatorState` (fill), `yellowLedOn`.

Add/rename: `whiteLedOn` only (or `lidOpen` + `white_on` derived).

Trim `[app]` and heartbeat Serial output accordingly.

### 6.7 `http_api.cpp`

Remove JSON keys:

- `waste` object
- `fill` object
- `leds.indicator` (fill NORMAL/WARNING/UNKNOWN)
- `leds.yellow_on`

Keep / simplify:

- `approach.detected`
- `lid.*`
- `leds.white_on` (and optionally `leds.indicator` as `OPEN`/`CLOSED` if clients need a pill)

Consider bumping `features` in `/api/health` (currently `monitoring_only`) to reflect simplified product — **API breaking change** for dashboard JS only in-repo.

### 6.8 `dashboard_page.cpp`

Remove:

- Fill level card (%, bar, fill state pill from `leds.indicator`)
- Yellow LED row
- Waste sensor card
- JS: `mapFillPill`, fill bar width, waste distance fields

Retain / extend:

- Approach, lid, close delay, servo angle, white LED, network, uptime

### 6.9 Documentation

| Document | Update type |
|----------|-------------|
| `README.md` | Rewrite product description, status table, GPIO table, architecture tree, remove fill/yellow/buzzer-as-fill features |
| `docs/HARDWARE_GPIO_MAP.md` | Mark waste sensor + yellow LED **removed from product scope** (or repurposed unconnected); update LED behaviour table |
| `docs/CALIBRATION_LOG.md` | Remove or archive fill empty/full procedures; keep approach + servo sections |
| `docs/SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md` | Add deprecation banner or successor pointer — historical |
| `docs/WP-0` … `WP-9` completion reports | **Historical** — do not delete; optional index note in README |
| `docs/CASE-20261119-DUSTBIN-LED-D0-INVESTIGATION.md` | Historical; explains fill-driven white LED — motivates WP-11 LED redesign |

No requirement to rewrite all WP completion PDFs; **README + HARDWARE_GPIO_MAP** are mandatory for “active features” accuracy.

---

## 7. Waste-monitoring dependency graph

```text
waste_sensor ──────┐
                   ├──► waste_fill_level ──► led_status ──► GPIO D0/D3
ultrasonic_scheduler ◄── waste_sensor      │
       ▲                                   │
       └── approach_sensor (shared quiet) │
gpio_init (D5/D6) ──► waste_sensor         │
config.h (refs, warn %) ───────────────────┘
                   │
                   └──► application_status ──► http_api / dashboard
SmartDustbin.ino (loop branch, banner, heartbeat)
```

**Cut set for waste removal:** deleting `waste_sensor`, `waste_fill_level`, scheduler waste branch, and rewiring `led_status` breaks all downstream consumers in one pass if `application_status` and HTTP are updated in the same WP.

---

## 8. Shared code — preserve vs refactor

| Asset | Action |
|-------|--------|
| `hcsr04_common.*` | **Preserve** — single sensor still needs it |
| `approach_sensor.*` | **Preserve** — core product |
| `lid_control.*` | **Preserve** — matches target behaviour |
| `ultrasonic_scheduler.*` | **Remove or slim** — only consumer of waste path |
| `led_status.*` | **Heavy refactor** — new semantics |
| `wifi_manager.*` | **Preserve** |
| `http_api.*` / `dashboard_page.*` | **Trim** |
| Servo library | **Preserve** (ESP8266 core) |

---

## 9. Scheduler and timing — waste removal effects

| Topic | Finding |
|-------|---------|
| Dual-sensor alternation | `s_serveWasteNext` and `CONFIG_WASTE_SAMPLE_MS` become dead code |
| Approach sample rate | Today approach can be **delayed** when waste wins arbitration and during 60 ms quiet windows. Approach-only → **more predictable** ~100 ms + quiet |
| Blocking `pulseIn` | One sensor reduces worst-case loop block per cycle |
| Lid close delay | Uses `millis()` in `lid_control` — **independent** of scheduler |
| Wi-Fi / HTTP | Still serviced each loop; less ultrasonic work → **slightly more** CPU margin |
| `CONFIG_ULTRASONIC_QUIET_MS` | Still useful between approach pulses (acoustic settling) |

**Regression risk:** Lower — removal should not break lid timing; may **improve** approach responsiveness.

---

## 10. Potential regressions and risks

| ID | Risk | Severity | Mitigation |
|----|------|----------|------------|
| R-1 | White LED stays off after simplification if waste removed before LED reparented to lid | **High** | Implement LED/lid binding in same WP as waste removal |
| R-2 | API/dashboard clients expect `fill` / `waste` JSON | Medium | Coordinate dashboard + API in one WP; document breaking change |
| R-3 | D3 GPIO0 left floating vs driven LOW | Low–Med | Document hardware: leave yellow unwired; avoid OUTPUT LOW on GPIO0 if pin unused |
| R-4 | D5/D6 floating inputs if waste sensor unplugged | Low | No `pinMode` or explicit INPUT without pull if pins removed from `gpio_init` |
| R-5 | Loop order: lid uses stale approach flag | Low (existing) | Document; optional future reorder (approach measure before lid) — separate decision |
| R-6 | White ON during close delay | Low | Use `LidPositionState::Open`, not `approachDetected` alone |
| R-7 | Serial log volume / field removal breaks Steven’s test scripts | Low | Publish new heartbeat format in WP completion report |
| R-8 | IRAM pressure (93% at WP-9 build) | Low | Waste removal may **reclaim** flash/IRAM slightly |
| R-9 | README still says features “Planned” while WP-9 delivered | Low | WP-11 doc pass fixes product messaging |

---

## 11. Proposed implementation work packages

Independently testable sequence (recommended). Each WP ends with `arduino-cli compile` and bench checklist — **no upload in agent scope unless assigned**.

### WP-11.1 — Core excision (firmware backend)

**Scope:** Remove waste modules and scheduler waste path; simplify approach timing; strip `gpio_init` waste pins; remove waste config symbols.

**Files:** `waste_sensor.*` (delete), `waste_fill_level.*` (delete), `ultrasonic_scheduler.*` (delete or slim), `gpio_init.cpp`, `config.h`, `pins.h`, `SmartDustbin.ino` (partial).

**Acceptance criteria:**

- Build succeeds with zero references to `waste_` or `CONFIG_WASTE_*` / fill refs.
- Approach measurements still run on ~`CONFIG_APPROACH_SAMPLE_MS` with quiet guard.
- D5/D6 not initialized.
- Serial shows `[approach]` only (no `[waste]`).
- Lid open/close behaviour unchanged on bench.

**Validation:** Serial approach detect; lid open/close/delay/cancel — **white may still be wrong** until WP-11.2.

---

### WP-11.2 — White LED tied to lid state

**Scope:** Refactor `led_status` to drive only D0 from `lidControlGetLidState()`; remove yellow and fill types.

**Files:** `led_status.cpp/h`, `SmartDustbin.ino` (`ledStatusUpdate` call order after `lidControlUpdate`).

**Acceptance criteria:**

- Boot: white OFF, lid closed.
- Approach → lid open → white ON.
- Leave zone → delay → lid close → white OFF.
- Re-approach during delay → lid stays open, white stays ON.
- Serial `[led]` or heartbeat shows `white=1` iff lid OPEN.

**Validation:** GPIO16 meter or visual; matches target behaviours 1–6.

---

### WP-11.3 — Status, API, and dashboard

**Scope:** `application_status`, `http_api`, `dashboard_page` aligned to simplified JSON.

**Files:** `application_status.*`, `http_api.cpp`, `dashboard_page.cpp`.

**Acceptance criteria:**

- `GET /api/status` has no `waste`, `fill`, `yellow_on`, or fill-style `indicator`.
- Dashboard renders approach, lid, white, network without errors in browser console.
- `/api/health` still returns 200.

**Validation:** curl/browser on LAN; JSON schema spot-check.

---

### WP-11.4 — Serial / orchestration cleanup

**Scope:** Boot banner, heartbeat, `[app]` diagnostic fields; optional rename WP banner string in `SmartDustbin.ino`.

**Acceptance criteria:**

- No fill/waste/warn strings in banner or 5 s heartbeat.
- `[app]` includes approach, lid, white only (agreed field set).

**Validation:** 60 s Serial capture during approach cycles.

---

### WP-11.5 — Documentation and hardware map

**Scope:** `README.md`, `HARDWARE_GPIO_MAP.md`, `CALIBRATION_LOG.md`; optional `docs/WP-11_COMPLETION_REPORT.md`.

**Acceptance criteria:**

- README describes **approach-and-open** product only.
- Hardware map lists one HC-SR04, white LED, servo, buzzer noted optional/future.
- No doc states fill % or yellow warning as active features.

**Validation:** Steven/Craig doc review sign-off.

---

## 12. Recommended validation sequence (end-to-end)

1. **Compile** — `arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin` after each WP.
2. **Cold boot** — lid closed, white off, approach sensor idle TRIG low.
3. **Approach** — enter ≤ detect cm: `[approach] detected=1`, `[lid] opening`, white on.
4. **Hold** — lid open, white on.
5. **Leave** — `detected=0`, `[lid] close_delay_start`, white **still on** during delay.
6. **Re-approach before close** — `close_delay_cancelled`, lid open, white on.
7. **Let delay finish** — `[lid] closing`, white off.
8. **Wi-Fi** — dashboard loads; status JSON matches physical state.
9. **Regression** — Wi-Fi drop/reconnect; HTTP while servo moves; no watchdog reset during continuous approach cycling (5 min soak).

Record results in a new completion report (post-implementation).

---

## 13. Files and modules — consolidated matrix

| File | WP-11 action |
|------|----------------|
| `SmartDustbin.ino` | Modify — remove waste/fill; simplify scheduler; heartbeat |
| `config.h` | Modify — remove waste/fill/warn |
| `pins.h` | Modify — remove waste + yellow defines |
| `gpio_init.cpp` | Modify — approach pins only |
| `gpio_init.h` | Keep |
| `hcsr04_common.*` | Keep |
| `approach_sensor.*` | Keep |
| `waste_sensor.*` | **Delete** |
| `waste_fill_level.*` | **Delete** |
| `ultrasonic_scheduler.*` | **Delete or replace** |
| `lid_control.*` | Keep |
| `led_status.*` | **Major refactor** |
| `application_status.*` | Modify |
| `http_api.*` | Modify |
| `dashboard_page.*` | Modify |
| `wifi_manager.*` | Keep |
| `lid_control.h` | Keep |
| `README.md` | Modify |
| `docs/HARDWARE_GPIO_MAP.md` | Modify |
| `docs/CALIBRATION_LOG.md` | Modify |
| `docs/WP-*` (0–9), audits | Keep as history |

---

## 14. Acceptance criteria — simplified product (release gate)

| ID | Criterion |
|----|-----------|
| AC-1 | Single HC-SR04 (approach) measured in firmware |
| AC-2 | Lid opens on approach, closes after delay when clear |
| AC-3 | Re-approach during delay cancels close and keeps lid open |
| AC-4 | White LED on D0 ON whenever lid is open (including close delay) |
| AC-5 | White LED OFF when lid closed |
| AC-6 | No fill %, waste distance, or yellow LED in firmware, API, or dashboard |
| AC-7 | Wi-Fi and local dashboard operational |
| AC-8 | Documentation matches simplified scope |
| AC-9 | Clean compile on nodemcuv2 FQBN |

---

## 15. Open decisions for Steven / Craig

1. **Rename `led_status`** → e.g. `white_indicator` vs refactor in place?
2. **Delete `ultrasonic_scheduler`** vs inline approach timing in `.ino`?
3. **`/api/status` compatibility** — breaking JSON vs versioned endpoint?
4. **Physical hardware** — disconnect waste sensor and yellow LED, or leave on bench unrouted?
5. **Buzzer** — remain out of scope (current) or future WP on fill-transition removal?

---

## 16. Audit restrictions compliance

| Constraint | Status |
|------------|--------|
| No source/config edits during audit | **Complied** (this document only) |
| No commit/push/upload | **Complied** |
| Git clean at audit start | **Complied** |
| Implementation awaits approval | **Stated** |

---

**End of WP-11 read-only impact audit. Implementation must not begin until Steven and Craig approve this plan.**