from __future__ import annotations

import logging
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Callable

from smartdustbin_gateway.config import GatewayConfig
from smartdustbin_gateway.cors import (
    cors_response_headers,
    evaluate_origin,
    preflight_headers_are_valid,
    preflight_response_headers,
)
from smartdustbin_gateway.proxy import (
    UpstreamConnectionError,
    UpstreamProtocolError,
    UpstreamTimeoutError,
    proxy_request,
)
from smartdustbin_gateway.routing import RouteDecision, classify_request, preflight_allowed_methods_for_path

_LOG = logging.getLogger(__name__)


def create_request_handler(
    config: GatewayConfig,
    proxy_fn: Callable[..., tuple[int, list[tuple[str, str]], bytes]] | None = None,
) -> type[BaseHTTPRequestHandler]:
    upstream = proxy_fn or proxy_request

    class GatewayHandler(BaseHTTPRequestHandler):
        server_version = "SmartDustbinGateway/1.0"

        def log_message(self, format: str, *args) -> None:  # noqa: A003
            _LOG.info("%s - %s", self.address_string(), format % args)

        def handle_one_request(self) -> None:
            try:
                super().handle_one_request()
            except (ConnectionResetError, BrokenPipeError):
                pass

        def _origin_state(self) -> str:
            return evaluate_origin(self.headers.get("Origin"))

        def _merge_cors_headers(
            self,
            headers: list[tuple[str, str]],
            origin_state: str,
        ) -> list[tuple[str, str]]:
            if origin_state != "allowed":
                return headers
            merged = list(headers)
            for name, value in cors_response_headers():
                if not any(existing[0].lower() == name.lower() for existing in merged):
                    merged.append((name, value))
            return merged

        def _read_body(self) -> bytes | None:
            length_header = self.headers.get("Content-Length")
            if length_header is None:
                if self.command.upper() in {"POST", "PUT", "PATCH"}:
                    return b""
                return None
            try:
                length = int(length_header)
            except ValueError:
                self._send_gateway_error(400, "Invalid Content-Length")
                return None
            if length < 0:
                self._send_gateway_error(400, "Invalid Content-Length")
                return None
            if length > config.max_body_bytes:
                self._send_gateway_error(413, "Request body too large")
                return None
            if length == 0:
                return b""
            return self.rfile.read(length)

        def _send_response(
            self,
            status: int,
            headers: list[tuple[str, str]],
            body: bytes,
        ) -> None:
            origin_state = self._origin_state()
            headers = self._merge_cors_headers(headers, origin_state)
            self.send_response(status)
            for name, value in headers:
                self.send_header(name, value)
            if not any(h[0].lower() == "content-length" for h in headers):
                self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            if body:
                self.wfile.write(body)

        def _send_gateway_error(self, status: int, message: str) -> None:
            body = message.encode("utf-8")
            headers = [
                ("Content-Type", "text/plain; charset=utf-8"),
                ("Content-Length", str(len(body))),
                ("Cache-Control", "no-store"),
            ]
            self._send_response(status, headers, body)

        def _send_client_response(
            self,
            status: int,
            headers: list[tuple[str, str]],
            body: bytes,
        ) -> None:
            if not any(h[0].lower() == "content-length" for h in headers):
                headers = list(headers)
                headers.append(("Content-Length", str(len(body))))
            if not any(h[0].lower() == "cache-control" for h in headers):
                headers = list(headers)
                headers.append(("Cache-Control", "no-store"))
            self._send_response(status, headers, body)

        def _reject_disallowed_origin(self) -> bool:
            if self._origin_state() == "denied":
                self._send_gateway_error(403, "Origin not allowed")
                return True
            return False

        def _handle_options(self) -> None:
            if self._origin_state() != "allowed":
                self._send_gateway_error(403, "Origin not allowed")
                return

            allowed_on_route = preflight_allowed_methods_for_path(self.path)
            if allowed_on_route is None:
                self._send_gateway_error(404, "Not found")
                return

            requested_method = self.headers.get("Access-Control-Request-Method")
            requested_headers = self.headers.get("Access-Control-Request-Headers")
            method = requested_method.strip().upper() if requested_method else ""
            if method not in allowed_on_route:
                self._send_gateway_error(405, "Method not allowed")
                return
            if not preflight_headers_are_valid(requested_headers):
                self._send_gateway_error(403, "Header not allowed")
                return

            headers = preflight_response_headers(allowed_on_route)
            headers.append(("Content-Length", "0"))
            self._send_response(204, headers, b"")

        def _dispatch(self) -> None:
            if self._reject_disallowed_origin():
                return

            decision, upstream_path = classify_request(self.command, self.path)
            if decision is RouteDecision.NOT_FOUND:
                self._send_gateway_error(404, "Not found")
                return
            if decision is RouteDecision.METHOD_NOT_ALLOWED:
                self._send_gateway_error(405, "Method not allowed")
                return
            if upstream_path is None:
                self._send_gateway_error(404, "Not found")
                return

            body = self._read_body()
            if body is None and self.command.upper() in {"POST", "PUT", "PATCH"}:
                if self.headers.get("Content-Length") is not None:
                    return

            header_pairs = list(self.headers.items())
            try:
                status, resp_headers, resp_body = upstream(
                    config,
                    self.command,
                    upstream_path,
                    header_pairs,
                    body,
                )
            except UpstreamTimeoutError:
                self._send_gateway_error(504, "Upstream timed out")
                return
            except UpstreamConnectionError:
                self._send_gateway_error(502, "Upstream unavailable")
                return
            except UpstreamProtocolError:
                self._send_gateway_error(502, "Upstream error")
                return
            except ValueError:
                self._send_gateway_error(413, "Request body too large")
                return

            self._send_client_response(status, resp_headers, resp_body)

        def do_GET(self) -> None:
            self._dispatch()

        def do_POST(self) -> None:
            if self._reject_disallowed_origin():
                return
            self._send_gateway_error(405, "Method not allowed")

        def do_PUT(self) -> None:
            if self._reject_disallowed_origin():
                return
            self._send_gateway_error(405, "Method not allowed")

        def do_PATCH(self) -> None:
            if self._reject_disallowed_origin():
                return
            self._send_gateway_error(405, "Method not allowed")

        def do_DELETE(self) -> None:
            if self._reject_disallowed_origin():
                return
            self._send_gateway_error(405, "Method not allowed")

        def do_OPTIONS(self) -> None:
            self._handle_options()

        def do_HEAD(self) -> None:
            if self._reject_disallowed_origin():
                return
            self._send_gateway_error(405, "Method not allowed")

    return GatewayHandler


def run_server(config: GatewayConfig, proxy_fn: Callable[..., tuple[int, list[tuple[str, str]], bytes]] | None = None) -> ThreadingHTTPServer:
    handler = create_request_handler(config, proxy_fn=proxy_fn)
    server = ThreadingHTTPServer((config.listen_host, config.listen_port), handler)
    return server