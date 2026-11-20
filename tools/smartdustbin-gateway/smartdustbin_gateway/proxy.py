from __future__ import annotations

import http.client
import logging
import socket
from typing import Iterable

from smartdustbin_gateway.config import GatewayConfig

_LOG = logging.getLogger(__name__)

_REQUEST_HEADER_ALLOWLIST = frozenset(
    {
        "content-type",
        "accept",
        "accept-encoding",
        "accept-language",
        "content-length",
    }
)

_RESPONSE_HEADER_ALLOWLIST = frozenset(
    {
        "content-type",
        "cache-control",
        "content-length",
    }
)


class UpstreamError(Exception):
    """Base class for gateway upstream failures."""


class UpstreamConnectionError(UpstreamError):
    pass


class UpstreamTimeoutError(UpstreamError):
    pass


class UpstreamProtocolError(UpstreamError):
    pass


def _filtered_headers(headers: Iterable[tuple[str, str]], allowlist: frozenset[str]) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for name, value in headers:
        if name.lower() in allowlist:
            out.append((name, value))
    return out


def proxy_request(
    config: GatewayConfig,
    method: str,
    path: str,
    headers: list[tuple[str, str]],
    body: bytes | None,
) -> tuple[int, list[tuple[str, str]], bytes]:
    """Forward one HTTP request to the ESP8266 and return status, headers, body."""
    if body is not None and len(body) > config.max_body_bytes:
        raise ValueError("request body too large")

    path_only = path.split("?", 1)[0]
    target = path if path.startswith("/") else f"/{path}"

    forward_headers: dict[str, str] = {}
    for name, value in _filtered_headers(headers, _REQUEST_HEADER_ALLOWLIST):
        forward_headers[name] = value

    if body is not None and body:
        forward_headers["Content-Length"] = str(len(body))

    conn: http.client.HTTPConnection | None = None
    try:
        try:
            sock = socket.create_connection(
                (config.upstream_host, config.upstream_port),
                timeout=config.connect_timeout_s,
            )
        except (TimeoutError, socket.timeout, OSError) as exc:
            _LOG.warning("upstream connection failed host=%s path=%s", config.upstream_host, path_only)
            raise UpstreamConnectionError(str(exc)) from exc
        sock.settimeout(config.read_timeout_s)
        conn = http.client.HTTPConnection(config.upstream_host, config.upstream_port)
        conn.sock = sock
        conn.request(method.upper(), target, body=body, headers=forward_headers)
        response = conn.getresponse()
        response_body = response.read()
        status = response.status
        resp_headers = _filtered_headers(response.getheaders(), _RESPONSE_HEADER_ALLOWLIST)
        return status, resp_headers, response_body
    except socket.timeout as exc:
        _LOG.warning("upstream timeout host=%s path=%s", config.upstream_host, path_only)
        raise UpstreamTimeoutError(str(exc)) from exc
    except OSError as exc:
        _LOG.warning("upstream connection failed host=%s path=%s", config.upstream_host, path_only)
        raise UpstreamConnectionError(str(exc)) from exc
    except http.client.HTTPException as exc:
        _LOG.warning("upstream protocol error host=%s path=%s", config.upstream_host, path_only)
        raise UpstreamProtocolError(str(exc)) from exc
    finally:
        if conn is not None:
            conn.close()