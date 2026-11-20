from __future__ import annotations

import http.client
import socket
import threading
import time
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

from smartdustbin_gateway.config import GatewayConfig, config_from_mapping, load_config
from smartdustbin_gateway.cors import ALLOWED_ORIGIN
from smartdustbin_gateway.proxy import UpstreamConnectionError, UpstreamTimeoutError, proxy_request
from smartdustbin_gateway.routing import RouteDecision, classify_request
from smartdustbin_gateway.server import run_server


class MockUpstreamHandler(BaseHTTPRequestHandler):
    last_request: dict | None = None
    options_received: bool = False
    delay_s: float = 0.0

    def log_message(self, format, *args) -> None:  # noqa: A003
        return

    def do_GET(self) -> None:
        self._handle()

    def do_OPTIONS(self) -> None:
        MockUpstreamHandler.options_received = True
        self.send_response(405)
        self.end_headers()

    def _handle(self) -> None:
        if self.delay_s > 0:
            time.sleep(self.delay_s)
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length > 0 else b""
        MockUpstreamHandler.last_request = {
            "method": self.command,
            "path": self.path,
            "headers": {k: v for k, v in self.headers.items()},
            "body": body,
        }
        if self.path == "/api/status":
            payload = b'{"approach":{"detected":false}}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
        elif self.path == "/api/health":
            payload = b'{"device":{"status":"ok"},"features":{"monitoring_only":true}}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
        else:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            payload = b"not found"

        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


def _free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


class RoutingTests(unittest.TestCase):
    def test_approved_get_routes(self) -> None:
        for path in ("/api/status", "/api/health"):
            decision, upstream = classify_request("GET", path)
            self.assertEqual(decision, RouteDecision.PROXY)
            self.assertEqual(upstream, path)

    def test_unknown_paths_rejected(self) -> None:
        self.assertEqual(classify_request("GET", "/")[0], RouteDecision.NOT_FOUND)
        self.assertEqual(classify_request("GET", "/api/unknown")[0], RouteDecision.NOT_FOUND)

    def test_unsupported_methods_on_api_rejected(self) -> None:
        self.assertEqual(classify_request("POST", "/api/status")[0], RouteDecision.METHOD_NOT_ALLOWED)
        self.assertEqual(classify_request("DELETE", "/api/health")[0], RouteDecision.METHOD_NOT_ALLOWED)


class GatewayIntegrationTests(unittest.TestCase):
    def setUp(self) -> None:
        MockUpstreamHandler.last_request = None
        MockUpstreamHandler.options_received = False
        MockUpstreamHandler.delay_s = 0.0
        self.upstream_port = _free_port()
        self.upstream = ThreadingHTTPServer(("127.0.0.1", self.upstream_port), MockUpstreamHandler)
        threading.Thread(target=self.upstream.serve_forever, daemon=True).start()

        self.gateway_port = _free_port()
        self.gateway_cfg = GatewayConfig(
            listen_port=self.gateway_port,
            upstream_host="127.0.0.1",
            upstream_port=self.upstream_port,
        )
        self.gateway = run_server(self.gateway_cfg)
        threading.Thread(target=self.gateway.serve_forever, daemon=True).start()

    def tearDown(self) -> None:
        self.gateway.shutdown()
        self.gateway.server_close()
        self.upstream.shutdown()
        self.upstream.server_close()

    def _request(
        self,
        method: str,
        path: str,
        headers: dict[str, str] | None = None,
    ) -> tuple[int, dict[str, str], bytes]:
        conn = http.client.HTTPConnection("127.0.0.1", self.gateway_port, timeout=5)
        conn.request(method, path, headers=headers or {})
        response = conn.getresponse()
        body = response.read()
        resp_headers = {name: value for name, value in response.getheaders()}
        conn.close()
        return response.status, resp_headers, body

    def test_get_api_status(self) -> None:
        status, headers, body = self._request("GET", "/api/status", {"Origin": ALLOWED_ORIGIN})
        self.assertEqual(status, 200)
        self.assertIn(b"approach", body)
        self.assertEqual(headers.get("Access-Control-Allow-Origin"), ALLOWED_ORIGIN)

    def test_get_api_health(self) -> None:
        status, headers, body = self._request("GET", "/api/health", {"Origin": ALLOWED_ORIGIN})
        self.assertEqual(status, 200)
        self.assertIn(b"monitoring_only", body)
        self.assertEqual(headers.get("Access-Control-Allow-Origin"), ALLOWED_ORIGIN)

    def test_options_preflight_status(self) -> None:
        status, headers, body = self._request(
            "OPTIONS",
            "/api/status",
            {
                "Origin": ALLOWED_ORIGIN,
                "Access-Control-Request-Method": "GET",
            },
        )
        self.assertEqual(status, 204)
        self.assertEqual(body, b"")
        self.assertEqual(headers.get("Access-Control-Allow-Methods"), "GET")
        self.assertFalse(MockUpstreamHandler.options_received)

    def test_options_preflight_health(self) -> None:
        status, headers, _ = self._request(
            "OPTIONS",
            "/api/health",
            {
                "Origin": ALLOWED_ORIGIN,
                "Access-Control-Request-Method": "GET",
            },
        )
        self.assertEqual(status, 204)
        self.assertEqual(headers.get("Access-Control-Allow-Origin"), ALLOWED_ORIGIN)
        self.assertFalse(MockUpstreamHandler.options_received)

    def test_unapproved_origin_rejected(self) -> None:
        MockUpstreamHandler.last_request = None
        status, headers, _ = self._request(
            "GET",
            "/api/status",
            {"Origin": "https://evil.example"},
        )
        self.assertEqual(status, 403)
        self.assertNotIn("Access-Control-Allow-Origin", headers)
        self.assertIsNone(MockUpstreamHandler.last_request)

    def test_options_missing_origin_rejected(self) -> None:
        status, headers, _ = self._request(
            "OPTIONS",
            "/api/status",
            {"Access-Control-Request-Method": "GET"},
        )
        self.assertEqual(status, 403)
        self.assertNotIn("Access-Control-Allow-Origin", headers)
        self.assertFalse(MockUpstreamHandler.options_received)

    def test_unsupported_method_post(self) -> None:
        MockUpstreamHandler.last_request = None
        status, _, _ = self._request(
            "POST",
            "/api/status",
            {"Origin": ALLOWED_ORIGIN, "Content-Type": "application/json"},
        )
        self.assertEqual(status, 405)
        self.assertIsNone(MockUpstreamHandler.last_request)

    def test_unknown_route_not_forwarded(self) -> None:
        MockUpstreamHandler.last_request = None
        status, _, _ = self._request("GET", "/")
        self.assertEqual(status, 404)
        self.assertIsNone(MockUpstreamHandler.last_request)

    def test_upstream_failure_returns_502(self) -> None:
        bad_port = _free_port()
        bad_cfg = GatewayConfig(
            listen_port=_free_port(),
            upstream_host="127.0.0.1",
            upstream_port=bad_port,
            connect_timeout_s=0.5,
            read_timeout_s=0.5,
        )
        server = run_server(bad_cfg)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            conn = http.client.HTTPConnection("127.0.0.1", bad_cfg.listen_port, timeout=5)
            conn.request("GET", "/api/status", headers={"Origin": ALLOWED_ORIGIN})
            response = conn.getresponse()
            status = response.status
            headers = {n: v for n, v in response.getheaders()}
            response.read()
            conn.close()
            self.assertEqual(status, 502)
            self.assertEqual(headers.get("Access-Control-Allow-Origin"), ALLOWED_ORIGIN)
        finally:
            server.shutdown()
            server.server_close()

    def test_gateway_binds_localhost_only(self) -> None:
        host, _ = self.gateway.server_address
        self.assertEqual(host, "127.0.0.1")
        self.assertEqual(self.gateway_cfg.listen_host, "127.0.0.1")


class ProxyUnitTests(unittest.TestCase):
    def setUp(self) -> None:
        MockUpstreamHandler.last_request = None
        self.upstream_port = _free_port()
        self.upstream = ThreadingHTTPServer(("127.0.0.1", self.upstream_port), MockUpstreamHandler)
        threading.Thread(target=self.upstream.serve_forever, daemon=True).start()

    def tearDown(self) -> None:
        self.upstream.shutdown()
        self.upstream.server_close()

    def test_upstream_timeout(self) -> None:
        MockUpstreamHandler.delay_s = 1.5
        cfg = GatewayConfig(
            upstream_host="127.0.0.1",
            upstream_port=self.upstream_port,
            connect_timeout_s=1.0,
            read_timeout_s=0.3,
        )
        with self.assertRaises(UpstreamTimeoutError):
            proxy_request(cfg, "GET", "/api/status", [], None)
        MockUpstreamHandler.delay_s = 0.0

    def test_connection_failure(self) -> None:
        port = _free_port()
        cfg = GatewayConfig(
            upstream_host="127.0.0.1",
            upstream_port=port,
            connect_timeout_s=0.2,
            read_timeout_s=0.2,
        )
        with self.assertRaises(UpstreamConnectionError):
            proxy_request(cfg, "GET", "/api/health", [], None)


class ConfigTests(unittest.TestCase):
    def test_rejects_non_localhost_bind(self) -> None:
        with self.assertRaises(ValueError):
            config_from_mapping({"listen_host": "0.0.0.0"})

    def test_load_example_mapping(self) -> None:
        example = Path(__file__).with_name("config.example.json")
        cfg = load_config(example)
        self.assertEqual(cfg.listen_host, "127.0.0.1")
        self.assertEqual(cfg.listen_port, 8788)


if __name__ == "__main__":
    unittest.main()