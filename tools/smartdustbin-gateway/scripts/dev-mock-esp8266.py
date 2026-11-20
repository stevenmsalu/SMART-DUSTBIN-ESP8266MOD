"""Minimal ESP8266 stand-in for gateway/tunnel validation (stdlib only). Not for production."""
from __future__ import annotations

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class Handler(BaseHTTPRequestHandler):
    def log_message(self, format, *args) -> None:  # noqa: A003
        return

    def do_GET(self) -> None:
        if self.path == "/api/health":
            body = json.dumps(
                {
                    "device": {"status": "ok", "uptime_ms": 1},
                    "wifi": {"connected": True, "ip": "127.0.0.1"},
                    "features": {"monitoring_only": True},
                }
            ).encode()
        elif self.path == "/api/status":
            body = json.dumps(
                {
                    "device": {"uptime_ms": 1},
                    "approach": {"detected": False},
                    "lid": {"state": "CLOSED", "close_delay_pending": False, "servo_target_angle_deg": 30},
                    "leds": {"white_on": False},
                    "network": {"wifi_connected": True, "ip": "127.0.0.1"},
                }
            ).encode()
        else:
            self.send_error(404)
            return

        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main() -> None:
    host = "127.0.0.1"
    port = 8899
    server = ThreadingHTTPServer((host, port), Handler)
    print(f"Mock ESP8266 http://{host}:{port} (/api/health, /api/status)", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()