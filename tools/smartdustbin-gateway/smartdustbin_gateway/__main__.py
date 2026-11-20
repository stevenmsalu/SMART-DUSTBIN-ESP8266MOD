from __future__ import annotations

import argparse
import logging
import signal
import sys
from pathlib import Path

from smartdustbin_gateway.config import load_config
from smartdustbin_gateway.server import run_server


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Smart Dustbin API gateway (localhost only)")
    parser.add_argument(
        "--config",
        type=Path,
        help="Path to gateway config JSON (see config.example.json)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable debug logging",
    )
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )

    try:
        config = load_config(args.config)
    except (OSError, ValueError) as exc:
        logging.error("Failed to load configuration: %s", exc)
        return 1

    server = run_server(config)
    host, port = server.server_address
    logging.info(
        "Smart Dustbin gateway listening on http://%s:%s -> %s:%s",
        host,
        port,
        config.upstream_host,
        config.upstream_port,
    )

    def shutdown(_signum, _frame) -> None:
        logging.info("Shutting down gateway")
        server.shutdown()

    signal.signal(signal.SIGINT, shutdown)
    if hasattr(signal, "SIGTERM"):
        signal.signal(signal.SIGTERM, shutdown)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())