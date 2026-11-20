(() => {
  "use strict";

  const POLL_MS = 2500;
  const API_BASE_PLACEHOLDER = "YOUR-CURRENT-QUICK-TUNNEL";

  let busy = false;
  let timer = null;
  let stale = false;

  function resolveApiBase() {
    const cfg = window.SMART_DUSTBIN_CONFIG;
    const raw =
      cfg && cfg.API_BASE_URL != null ? String(cfg.API_BASE_URL).trim() : "";
    if (!raw || raw.includes(API_BASE_PLACEHOLDER)) {
      return "";
    }
    return raw.replace(/\/+$/, "");
  }

  function isSameOriginApiHost(hostname) {
    return (
      hostname === "localhost" ||
      hostname === "127.0.0.1" ||
      /^10\./.test(hostname) ||
      /^192\.168\./.test(hostname) ||
      /^172\.(1[6-9]|2\d|3[01])\./.test(hostname) ||
      /^\d{1,3}(\.\d{1,3}){3}$/.test(hostname)
    );
  }

  function apiUrl(path) {
    const normalizedPath = path.startsWith("/") ? path : `/${path}`;
    const base = resolveApiBase();
    if (base) {
      return `${base}${normalizedPath}`;
    }
    if (isSameOriginApiHost(window.location.hostname)) {
      return normalizedPath;
    }
    throw new Error(
      "Smart Dustbin API URL is not configured. Set API_BASE_URL in config.js to your gateway Quick Tunnel URL."
    );
  }

  function setText(id, t) {
    const el = document.getElementById(id);
    if (el) {
      el.textContent = t;
    }
  }

  function setHtml(id, h) {
    const el = document.getElementById(id);
    if (el) {
      el.innerHTML = h;
    }
  }

  function chip(cls, label) {
    return `<span class="chip ${cls}">${label}</span>`;
  }

  function fmtUptime(ms) {
    if (ms == null) {
      return "—";
    }
    const s = Math.floor(ms / 1000);
    const h = Math.floor(s / 3600);
    const m = Math.floor((s % 3600) / 60);
    const r = s % 60;
    return `${h ? `${h}h ` : ""}${m ? `${m}m ` : ""}${r}s`;
  }

  function setConn(ok, msg) {
    const b = document.getElementById("conn");
    b.className = `banner ${ok ? "ok" : "bad"}`;
    b.textContent = msg;
  }

  function lidLabel(state) {
    if (state === "OPEN") {
      return "Open";
    }
    if (state === "CLOSED") {
      return "Closed";
    }
    return state || "—";
  }

  async function refresh() {
    if (busy) {
      return;
    }
    busy = true;
    try {
      const [hr, sr] = await Promise.all([
        fetch(apiUrl("/api/health")),
        fetch(apiUrl("/api/status")),
      ]);
      if (!hr.ok || !sr.ok) {
        throw new Error("http");
      }
      const health = await hr.json();
      const status = await sr.json();
      stale = false;
      const now = new Date().toLocaleTimeString();
      setConn(true, "Connected — view only");
      setText("lastUpd", now);
      const wifi = health.wifi && health.wifi.connected;
      setText("wifiState", wifi ? "Connected" : "Disconnected");
      const ip =
        wifi && health.wifi.ip
          ? health.wifi.ip
          : status.network && status.network.ip
            ? status.network.ip
            : null;
      setText("ipState", ip || "—");
      const up =
        status.device && status.device.uptime_ms != null
          ? status.device.uptime_ms
          : health.device && health.device.uptime_ms;
      setText("uptime", fmtUptime(up));
      const detected = !!(status.approach && status.approach.detected);
      setHtml(
        "nearby",
        detected
          ? chip("active", "Something detected")
          : chip("ok", "Nothing detected")
      );
      const lid = status.lid || {};
      setText("lidPos", lidLabel(lid.state));
      setHtml(
        "closingSoon",
        lid.close_delay_pending
          ? chip("wait", "Yes — lid will close")
          : chip("muted", "No")
      );
      const whiteOn = !!(status.leds && status.leds.white_on);
      setHtml(
        "indLight",
        whiteOn ? chip("active", "On") : chip("muted", "Off")
      );
      setText(
        "motorDeg",
        lid.servo_target_angle_deg != null
          ? `${lid.servo_target_angle_deg}°`
          : "—"
      );
    } catch (e) {
      if (!stale) {
        stale = true;
        if (e && e.message && e.message.includes("API URL is not configured")) {
          setConn(false, e.message);
        } else {
          setConn(false, "Connection lost — data may be outdated");
        }
      }
    } finally {
      busy = false;
    }
  }

  function start() {
    refresh();
    timer = setInterval(refresh, POLL_MS);
  }

  start();
})();