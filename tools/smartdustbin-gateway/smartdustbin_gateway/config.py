from __future__ import annotations

import json
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any

DEFAULT_LISTEN_HOST = "127.0.0.1"
DEFAULT_LISTEN_PORT = 8788
DEFAULT_UPSTREAM_HOST = "127.0.0.1"
DEFAULT_UPSTREAM_PORT = 80
DEFAULT_CONNECT_TIMEOUT_S = 3.0
DEFAULT_READ_TIMEOUT_S = 20.0
DEFAULT_MAX_BODY_BYTES = 65_536

ENV_CONFIG_PATH = "SMARTDUSTBIN_GATEWAY_CONFIG"
ENV_ESP8266_HOST = "SMARTDUSTBIN_ESP8266_HOST"
ENV_ESP8266_PORT = "SMARTDUSTBIN_ESP8266_PORT"
ENV_LISTEN_PORT = "SMARTDUSTBIN_GATEWAY_PORT"


@dataclass(frozen=True)
class GatewayConfig:
    listen_host: str = DEFAULT_LISTEN_HOST
    listen_port: int = DEFAULT_LISTEN_PORT
    upstream_host: str = DEFAULT_UPSTREAM_HOST
    upstream_port: int = DEFAULT_UPSTREAM_PORT
    connect_timeout_s: float = DEFAULT_CONNECT_TIMEOUT_S
    read_timeout_s: float = DEFAULT_READ_TIMEOUT_S
    max_body_bytes: int = DEFAULT_MAX_BODY_BYTES

    def validate(self) -> None:
        if self.listen_host != "127.0.0.1":
            raise ValueError("listen_host must be 127.0.0.1")
        if not (1 <= self.listen_port <= 65535):
            raise ValueError("listen_port out of range")
        if not self.upstream_host:
            raise ValueError("upstream_host is required")
        if not (1 <= self.upstream_port <= 65535):
            raise ValueError("upstream_port out of range")
        if self.connect_timeout_s <= 0 or self.read_timeout_s <= 0:
            raise ValueError("timeouts must be positive")
        if self.max_body_bytes <= 0:
            raise ValueError("max_body_bytes must be positive")


def _coerce_int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{field} must be an integer")
    return value


def _coerce_float(value: Any, field: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{field} must be a number")
    return float(value)


def config_from_mapping(data: dict[str, Any]) -> GatewayConfig:
    listen_host = str(data.get("listen_host", DEFAULT_LISTEN_HOST))
    listen_port = _coerce_int(data.get("listen_port", DEFAULT_LISTEN_PORT), "listen_port")
    upstream_host = str(data.get("upstream_host", DEFAULT_UPSTREAM_HOST))
    upstream_port = _coerce_int(data.get("upstream_port", DEFAULT_UPSTREAM_PORT), "upstream_port")
    connect_timeout_s = _coerce_float(
        data.get("connect_timeout_s", DEFAULT_CONNECT_TIMEOUT_S),
        "connect_timeout_s",
    )
    read_timeout_s = _coerce_float(
        data.get("read_timeout_s", DEFAULT_READ_TIMEOUT_S),
        "read_timeout_s",
    )
    max_body_bytes = _coerce_int(data.get("max_body_bytes", DEFAULT_MAX_BODY_BYTES), "max_body_bytes")
    cfg = GatewayConfig(
        listen_host=listen_host,
        listen_port=listen_port,
        upstream_host=upstream_host,
        upstream_port=upstream_port,
        connect_timeout_s=connect_timeout_s,
        read_timeout_s=read_timeout_s,
        max_body_bytes=max_body_bytes,
    )
    cfg.validate()
    return cfg


def load_config(config_path: Path | None = None) -> GatewayConfig:
    env_path = os.environ.get(ENV_CONFIG_PATH)
    if config_path is None and env_path:
        config_path = Path(env_path)

    data: dict[str, Any] = {}
    if config_path is not None:
        data = json.loads(config_path.read_text(encoding="utf-8"))
        if not isinstance(data, dict):
            raise ValueError("config file root must be a JSON object")

    env_host = os.environ.get(ENV_ESP8266_HOST)
    if env_host:
        data["upstream_host"] = env_host
    env_port = os.environ.get(ENV_ESP8266_PORT)
    if env_port:
        data["upstream_port"] = int(env_port)
    env_listen = os.environ.get(ENV_LISTEN_PORT)
    if env_listen:
        data["listen_port"] = int(env_listen)

    cfg = config_from_mapping(data)
    cfg.validate()
    return cfg