# Smart Dustbin API Gateway (WP-D-G2)

Lightweight localhost reverse proxy for the Smart Dustbin ESP8266 HTTP API (`GET /api/status`, `GET /api/health` only). Intended to sit behind **Cloudflare Tunnel** on the Dell laptop (`127.0.0.1` only).

## Requirements

- Python 3.10+ (stdlib only)
- ESP8266 reachable on the same LAN/hotspot (HTTP port 80)

## Quick start (PowerShell)

From this directory:

```powershell
cd SMART-DUSTBIN-ESP8266MOD\tools\smartdustbin-gateway
Copy-Item config.example.json config.json
# Edit config.json: set upstream_host to the ESP8266 IP from serial log / router
python -m smartdustbin_gateway --config config.json
```

Stop with `Ctrl+C`.

### Environment overrides

| Variable | Purpose |
|----------|---------|
| `SMARTDUSTBIN_GATEWAY_CONFIG` | Path to JSON config file |
| `SMARTDUSTBIN_ESP8266_HOST` | Upstream host (overrides config file) |
| `SMARTDUSTBIN_ESP8266_PORT` | Upstream port (default 80) |
| `SMARTDUSTBIN_GATEWAY_PORT` | Local listen port (default 8788) |

Example without editing the file:

```powershell
$env:SMARTDUSTBIN_ESP8266_HOST = "192.168.0.50"
python -m smartdustbin_gateway --config config.json
```

## Configuration

Copy `config.example.json` to `config.json` and set **`upstream_host`** when DHCP changes the ESP8266 address. `config.json` is gitignored — do not commit site-specific IPs.

| Field | Default | Meaning |
|-------|---------|---------|
| `listen_host` | `127.0.0.1` | Must stay loopback |
| `listen_port` | `8788` | Gateway port (RoomRadar uses **8787**) |
| `upstream_host` | (example) | ESP8266 IP on the LAN |
| `upstream_port` | `80` | Device HTTP |
| `connect_timeout_s` | `3` | TCP connect timeout |
| `read_timeout_s` | `20` | Upstream response timeout |
| `max_body_bytes` | `65536` | Max body (no bodies forwarded for GET-only API) |

## Allowed operations

Only these are forwarded; everything else returns **404** or **405** without contacting the ESP8266:

| Method | Path |
|--------|------|
| GET | `/api/status` |
| GET | `/api/health` |

**Not forwarded:** `GET /`, static files, POST/PUT/PATCH/DELETE, unknown `/api/*`.

## CORS

Browsers from **`https://kamanishasalle.pdinformationsystems.com`** receive exact-origin CORS headers on approved responses and errors. `OPTIONS` preflight is answered locally (GET only). Unapproved or missing `Origin` on preflight returns **403**. No credentials, no wildcard.

## Tests

```powershell
cd tools\smartdustbin-gateway
python -m unittest test_gateway.py -v
```

## Dell scripts (WP-D-G3)

From `tools\smartdustbin-gateway\scripts`:

| Script | Purpose |
|--------|---------|
| `Start-SmartDustbinStack.ps1` | Gateway window + tunnel window (waits for HTTP 200 `/api/health`) |
| `Start-SmartDustbinGateway.ps1` | Gateway only (`127.0.0.1:8788`) |
| `Start-SmartDustbinTunnel.ps1` | `cloudflared` → `http://127.0.0.1:8788` |
| `Verify-SmartDustbinLocal.ps1` | Local listener + `/api/health` + `/api/status` |
| `Validate-SmartDustbinTunnel.ps1` | Mock ESP8266 + gateway + tunnel E2E (cleans up its own processes) |

## PD dashboard link

After Quick Tunnel points at `http://127.0.0.1:8788`, set `API_BASE_URL` in `deployment/pd-hosting/config.js` to the tunnel URL (no trailing slash).

Public browser origin: `https://kamanishasalle.pdinformationsystems.com`