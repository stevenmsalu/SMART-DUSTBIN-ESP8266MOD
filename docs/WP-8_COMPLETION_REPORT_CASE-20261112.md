# WP-8 completion report — Wi-Fi connectivity & HTTP API

**Case ID:** CASE-20261112-SMARTDUSTBIN-END-TO-END  
**Work package:** WP-8  
**Date:** 2026-11-13  
**Target:** LoLin NodeMCU V3 — ESP8266 / ESP-12E

---

## 1. Implementation summary

WP-8 adds:

- **`wifi_manager`** — non-blocking STA connect, periodic reconnect (**10 s** backoff), connection state + IP query.
- **`http_api`** — **`ESP8266WebServer`** on port **80**, monitoring-only:
  - `GET /api/status` — JSON from **`ApplicationStatusSnapshot`** + Wi-Fi fields.
  - `GET /api/health` — lightweight JSON.

Credentials live in local **`wifi_config.h`** (gitignored); template **`wifi_config.h.example`** updated.

Sensors, lid, LEDs, scheduler, and serial diagnostics unchanged. Dustbin logic runs when Wi-Fi is down.

**Libraries:** `ESP8266WiFi`, `ESP8266WebServer` (ESP8266 Arduino Core — no extra packages).

---

## 2. Files created and modified

### Created

| File | Role |
|------|------|
| `SmartDustbin/wifi_manager.h` | Wi-Fi API |
| `SmartDustbin/wifi_manager.cpp` | Connect / reconnect |
| `SmartDustbin/http_api.h` | HTTP server API |
| `SmartDustbin/http_api.cpp` | Routes + streamed JSON |
| `docs/WP-8_COMPLETION_REPORT_CASE-20261112.md` | This report |

### Modified

| File | Change |
|------|--------|
| `SmartDustbin/config.h` | `CONFIG_WIFI_RECONNECT_INTERVAL_MS`, `CONFIG_HTTP_PORT` |
| `SmartDustbin/SmartDustbin.ino` | Init + loop: `wifiManagerUpdate`, `httpApiHandleClient` |
| `SmartDustbin/wifi_config.h.example` | WP-8 notes |

### Not modified

Subsystem measurement/fill/lid/LED logic; `application_status` aggregation; buzzer; GPIO.

### Credentials

**Do not commit** `SmartDustbin/wifi_config.h`. Copy from `.example` locally only.

---

## 3. Architecture

```text
loop()
  ├── lid / ultrasonic / LEDs  (unchanged)
  ├── wifiManagerUpdate()      (non-blocking)
  └── httpApiHandleClient()    (ESP8266WebServer)
```

HTTP handlers call **`applicationStatusGetSnapshot()`** only — no duplicate fill or sensor math.

JSON is **streamed** to the TCP client (`CONTENT_LENGTH_UNKNOWN`) — no large `String` / heap JSON buffer.

---

## 4. Wi-Fi credential configuration

1. Copy `SmartDustbin/wifi_config.h.example` → `SmartDustbin/wifi_config.h`
2. Set `WIFI_SSID` and `WIFI_PASSWORD` for your LAN
3. Confirm `wifi_config.h` is listed in `.gitignore`
4. Compile and upload from Arduino IDE / CLI

Hardware control does **not** depend on Wi-Fi.

---

## 5. Reconnection strategy

| Item | Behaviour |
|------|-----------|
| Initial | `WiFi.mode(WIFI_STA)`; `WiFi.begin()` once in `wifiManagerBegin()` |
| While connected | No periodic reconnect |
| On disconnect | Serial `[wifi] disconnected` |
| Retry | Earliest every **`CONFIG_WIFI_RECONNECT_INTERVAL_MS` (10000)** |
| Retry action | `WiFi.disconnect()` + `WiFi.begin()` (non-blocking) |
| Blocking | No `delay()` loops waiting for Wi-Fi |

`WiFi.setAutoReconnect(false)` — application backoff only (avoids hammering AP).

---

## 6. HTTP API endpoints

### GET `/api/status`

**Example (illustrative values):**

```json
{
  "device": { "uptime_ms": 123456 },
  "approach": { "detected": false },
  "waste": {
    "last_valid_distance_cm": { "available": true, "value": 28 },
    "latest_attempt": { "valid": true }
  },
  "fill": { "last_valid_percent": { "available": true, "value": 40 } },
  "lid": {
    "state": "CLOSED",
    "close_delay_pending": false,
    "servo_target_angle_deg": 30
  },
  "leds": {
    "indicator": "NORMAL",
    "white_on": true,
    "yellow_on": false
  },
  "network": { "wifi_connected": true, "ip": "192.168.1.50" }
}
```

When unavailable: `"available": false` and **no** fabricated `value` field.  
Invalid latest waste attempt: `"valid": false`, `"reason": "timeout"` or `"out_of_range"`.  
Wi-Fi down: `"wifi_connected": false`, `"ip": null`.

### GET `/api/health`

**Example:**

```json
{
  "device": { "status": "ok", "uptime_ms": 123456 },
  "wifi": { "connected": true, "ip": "192.168.1.50" },
  "features": { "monitoring_only": true }
}
```

---

## 7. Serial networking diagnostics

| Tag | When |
|-----|------|
| `[wifi] connect initiated` | Boot |
| `[wifi] connected ip=…` | First connect |
| `[wifi] disconnected` | Link lost |
| `[wifi] reconnect attempt` | Backoff retry |
| `[http] listening port 80` | Server start |
| `[http] GET /api/status` | Status request |
| `[http] GET /api/health` | Health request |

Existing `[approach]`, `[waste]`, `[lid]`, `[led]`, `[app]`, `[heartbeat]` preserved.

---

## 8. Compilation evidence

**Command:**

```text
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 SmartDustbin
```

| Metric | Value |
|--------|--------|
| **Exit code** | **0** |
| **Core** | esp8266:esp8266 **3.1.2** |
| **Flash (IROM)** | 279200 B used / 1048576 B (~26%) — **~769 KB headroom** |
| **RAM (static)** | 28692 B used / 80192 B (~36%) — **~51500 B** not in static BSS/DATA |
| **IRAM** | 61387 B / 65536 B (**~93%**) |
| **Data + BSS** | 1496 + 26160 (per build log) |

### Memory notes

- Wi-Fi + WebServer increased flash ~**36 KB** vs WP-7 build (~243 KB → ~279 KB IROM).
- Static RAM +~**450 B** vs WP-7 (~28236 → ~28692).
- **IRAM remains ~93%** (Servo + core + Wi-Fi stack) — watch before WP-9 dashboard assets or async stacks.
- JSON streaming avoids large heap allocations per request.

---

## 9. Known limitations

| ID | Limitation |
|----|------------|
| L-1 | No authentication — LAN-only assumption |
| L-2 | Port **80** fixed in `config.h` |
| L-3 | Single `ESP8266WebServer` instance — limited concurrent clients |
| L-4 | IP in JSON only when connected |
| L-5 | Physical Wi-Fi/API not validated in this WP |
| L-6 | High IRAM — risk if more network/UI code added without review |

---

## 10. Excluded functionality

Dashboard UI (WP-9), remote controls, buzzer (WP-10), cloud, auth, POST/config APIs, new sensors, GPIO or subsystem behaviour changes.

---

## 11. Skill selection

**Path:** `atlas-docs-project-source/Workspace Meta/.cursor.skills/` — **not available**.

**Selected skill:** None.

---

## 12. Physical validation (Steven — after upload)

1. Ensure `wifi_config.h` on device matches home AP.
2. Serial: `[wifi] connected ip=…`
3. Browser/curl: `http://<ip>/api/health` and `http://<ip>/api/status`
4. Confirm dustbin still operates with AP powered off.

---

**STOP** — WP-8 complete. Do not start WP-9 without approval.

*End of WP-8 completion report.*
