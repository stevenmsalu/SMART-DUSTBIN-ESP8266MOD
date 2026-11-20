from __future__ import annotations

from enum import Enum
from urllib.parse import urlparse

_EXACT_ROUTES: dict[str, frozenset[str]] = {
    "/api/status": frozenset({"GET"}),
    "/api/health": frozenset({"GET"}),
}


class RouteDecision(str, Enum):
    PROXY = "proxy"
    NOT_FOUND = "not_found"
    METHOD_NOT_ALLOWED = "method_not_allowed"


def _path_only(raw_path: str) -> str:
    parsed = urlparse(raw_path)
    return parsed.path or "/"


def is_approved_api_path(raw_path: str) -> bool:
    """True for gateway-approved API paths (used for CORS preflight)."""
    return preflight_allowed_methods_for_path(raw_path) is not None


def preflight_allowed_methods_for_path(raw_path: str) -> frozenset[str] | None:
    """HTTP methods permitted on this path (mirrors firmware route registration)."""
    path = _path_only(raw_path)
    if path in _EXACT_ROUTES:
        return _EXACT_ROUTES[path]
    return None


def classify_request(method: str, raw_path: str) -> tuple[RouteDecision, str | None]:
    """
    Decide whether to proxy. Returns (decision, upstream_path).
    upstream_path includes path + query unchanged when proxying.
    """
    method = method.upper()
    path = _path_only(raw_path)
    parsed = urlparse(raw_path)
    upstream_path = parsed.path or "/"
    if parsed.query:
        upstream_path = f"{upstream_path}?{parsed.query}"

    if path in _EXACT_ROUTES:
        allowed = _EXACT_ROUTES[path]
        if method not in allowed:
            return RouteDecision.METHOD_NOT_ALLOWED, None
        return RouteDecision.PROXY, upstream_path

    if path.startswith("/api/"):
        return RouteDecision.NOT_FOUND, None

    return RouteDecision.NOT_FOUND, None