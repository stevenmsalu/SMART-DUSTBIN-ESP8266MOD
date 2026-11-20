# Smart Dustbin — PD Information Systems static upload (WP-D-G1)

Upload **everything in this directory** to the web root for:

`https://kamanishasalle.pdinformationsystems.com`

## Package contents

| File | Purpose |
|------|---------|
| `index.html` | Dashboard homepage |
| `styles.css` | Styles (extracted from firmware `dashboard_page.cpp`) |
| `config.js` | **API base URL** (Cloudflare Quick Tunnel via gateway — WP-D-G2+) |
| `app.js` | Polling and UI logic |

No server-side runtime, database, or `.htaccess` is required.

## Local ESP8266 dashboard (unchanged)

The device still serves the embedded dashboard at `http://<device-ip>/` from firmware.  
No upload to PD is required for bench or LAN use.

This static package is for **external** hosting. When `API_BASE_URL` is empty or still contains the placeholder `YOUR-CURRENT-QUICK-TUNNEL`, the page uses **same-origin** relative URLs only on private LAN hostnames (e.g. if you mirror these files on the device network for testing). On the public PD site you must set a live tunnel URL after WP-D-G2.

## Before first upload

1. Complete **WP-D-G2** (Python gateway + Cloudflare Quick Tunnel) on the Dell.
2. Copy the `https://….trycloudflare.com` URL from the tunnel window.
3. Edit **`config.js`** and replace the placeholder:

   ```javascript
   API_BASE_URL: "https://your-actual-subdomain.trycloudflare.com"
   ```

   Use no trailing slash.

4. Upload all five files to PD hosting (overwrite existing).
5. Hard-refresh the dashboard in the browser (`Ctrl+F5`).

## When the Quick Tunnel URL changes

Quick Tunnel URLs change every time cloudflared restarts. **Only re-upload `config.js`** if `index.html`, `styles.css`, and `app.js` are unchanged.

1. Restart gateway + tunnel on the Dell.
2. Copy the new `trycloudflare.com` URL.
3. Update `API_BASE_URL` in `config.js`.
4. Upload `config.js` to PD hosting.
5. Refresh the dashboard.

The tunnel URL is **not** stored in firmware.

## Sync from firmware

When `SmartDustbin/src/dashboard_page.cpp` changes, refresh this folder from the embedded HTML/CSS/JS (and keep `config.js` with the live tunnel URL or placeholder).

```powershell
# Manual sync: compare dashboard_page.cpp DASHBOARD_HTML with index.html, styles.css, app.js
# Preserve config.js (do not overwrite live URL with placeholder).
```

## CORS

Browser requests from `https://kamanishasalle.pdinformationsystems.com` require the Dell gateway (WP-D-G2) to attach CORS headers. Firmware does not send CORS.