from __future__ import annotations

ALLOWED_ORIGIN = "https://kamanishasalle.pdinformationsystems.com"
ALLOWED_HEADERS = "Content-Type"
PREFLIGHT_ALLOW_HEADERS = frozenset({"content-type"})


def cors_vary_origin() -> tuple[str, str]:
    return ("Vary", "Origin")


def cors_allow_origin_header() -> tuple[str, str]:
    return ("Access-Control-Allow-Origin", ALLOWED_ORIGIN)


def cors_response_headers() -> list[tuple[str, str]]:
    return [cors_allow_origin_header(), cors_vary_origin()]


def preflight_response_headers(allowed_methods: frozenset[str]) -> list[tuple[str, str]]:
    methods = ", ".join(m for m in ("GET",) if m in allowed_methods)
    return [
        cors_allow_origin_header(),
        ("Access-Control-Allow-Methods", methods),
        ("Access-Control-Allow-Headers", ALLOWED_HEADERS),
        cors_vary_origin(),
    ]


def evaluate_origin(origin: str | None) -> str:
    """
    Returns: 'absent', 'allowed', or 'denied'.
    """
    if origin is None or origin.strip() == "":
        return "absent"
    if origin == ALLOWED_ORIGIN:
        return "allowed"
    return "denied"


def parse_requested_headers(raw: str | None) -> list[str] | None:
    if raw is None or raw.strip() == "":
        return []
    return [part.strip().lower() for part in raw.split(",") if part.strip()]


def preflight_headers_are_valid(requested_headers: str | None) -> bool:
    for header in parse_requested_headers(requested_headers):
        if header not in PREFLIGHT_ALLOW_HEADERS:
            return False
    return True