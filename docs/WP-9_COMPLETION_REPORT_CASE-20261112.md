# WP-9 completion report — Local monitoring dashboard

**Case ID:** CASE-20261112-SMARTDUSTBIN-END-TO-END  
**Work package:** WP-9  
**Date:** 2026-11-13  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-9 adds a **read-only** monitoring UI served at **`GET /`** from flash via **`send_P` / PROGMEM**.

- Single page: HTML + CSS + vanilla JS in **`dashboard_page.cpp`** (~6.8 KB flash delta vs WP-8).
- Polls **`/api/health`** and **`/api/status`** every **2500 ms** with a **busy** guard (no overlapping fetches).
- **No** CDN, frameworks, remote assets, or control endpoints.

**`GET /api/status`** and **`GET /api/health`** unchanged (WP-7 snapshot contract preserved).

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/dashboard_page.h` | PROGMEM HTML declaration |
| `SmartDustbin/dashboard_page.cpp` | Full dashboard document in flash |
| `docs/WP-9_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/http_api.cpp` | `GET /` → `handleDashboardRoot()` |
| `SmartDustbin/SmartDustbin.ino` | WP-9 banner strings |

### Unchanged

`wifi_manager`, `application_status`, all hardware subsystems, API JSON shape.

---

## 3. Dashboard serving strategy

| Aspect | Choice |
|--------|--------|
| Storage | `const char DASHBOARD_HTML[] PROGMEM` |
| Delivery | `ESP8266WebServer::send_P(200, "text/html", DASHBOARD_HTML)` |
| RAM | Page **not** copied to heap on serve |
| Stack | Existing `ESP8266WebServer` (no new libraries) |

---

## 4. Dashboard sections (requirements mapping)

| Section | UI | Data source |
|---------|-----|-------------|
| **A. Connection** | Banner + API/Wi‑Fi/IP/uptime/last update | `/api/health` + `/api/status` |
| **B. Fill** | %, progress bar, state pill | `fill.last_valid_percent`, `leds.indicator` (no JS threshold math) |
| **C. Approach** | CLEAR / DETECTED pill | `approach.detected` |
| **D. Lid** | state, servo°, close delay | `lid.*` |
| **E. LEDs** | indicator, white/yellow ON/OFF | `leds.*` |
| **F. Waste sensor** | last valid cm, latest valid, reason | `waste.last_valid_distance_cm`, `waste.latest_attempt` |

**Unavailable values** shown as “Unavailable” or “—” — **not** shown as `0`.

---

## 5. API polling and error handling

```text
POLL_MS = 2500
busy flag → skip tick if prior fetch in flight
Promise.all(health, status) on success
On failure → banner "API unavailable", apiState Unavailable (stale banner once)
On success after failure → auto recovery, green banner
No full page reload
```

---

## 6. Example data mapping

| JSON path | DOM |
|-----------|-----|
| `health.wifi.connected` | Wi‑Fi row |
| `health.wifi.ip` | IP row |
| `status.device.uptime_ms` | Uptime |
| `status.fill.last_valid_percent.available/value` | Fill % + bar width |
| `status.leds.indicator` | Fill state pill + LED indicator |
| `status.approach.detected` | Approach pill |
| `status.lid.state` | Lid state |
| `status.waste.last_valid_distance_cm` | Last distance |
| `status.waste.latest_attempt.reason` | Error reason when invalid |

---

## 7. Compilation evidence

**Command:**

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Metric | WP-8 (ref) | WP-9 | Δ |
|--------|------------|------|---|
| **Exit code** | 0 | **0** | — |
| **IROM flash** | 279200 B | **286040 B** | **+6840 B** |
| **Flash free (~)** | ~769 KB | **~762 KB** | −6.8 KB |
| **Static RAM** | 28692 B | **28712 B** | +20 B |
| **IRAM** | 61387 B (93%) | **61387 B (93%)** | 0 |

RODATA +12 B (1048 vs 1036). Dashboard lives primarily in **flash (IROM)**.

---

## 8. Known limitations

| ID | Note |
|----|------|
| L-1 | Browser must reach device IP on LAN (no captive portal) |
| L-2 | Stale UI on API errors — values not cleared aggressively |
| L-3 | Single-page size ~6–7 KB — further growth needs flash budget review |
| L-4 | IRAM still **93%** — no headroom for heavy async stacks |
| L-5 | Not tested on physical device in this WP |

---

## 9. Excluded functionality

No hardware controls, POST/PUT/DELETE, config UI, buzzer, cloud, external JS/CSS.

---

## 10. Skill selection

`atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not available**. **No skill used.**

---

## 11. Physical validation (Steven)

1. Connect phone/laptop to same LAN as NodeMCU.
2. Open `http://<device-ip>/`
3. Confirm sections update every ~2.5 s.
4. Disable Wi‑Fi briefly on AP — banner should show API unavailable; recover on restore.

---

**STOP** — WP-9 complete. Do not start WP-10 without approval.

*End of WP-9 completion report.*
