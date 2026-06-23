"""Dependency-free HTTP server for the BalanceBot companion app.

Uses only the Python standard library (no Flask/FastAPI needed), so it runs with
a bare ``python`` install. It exposes a small REST API, streams live telemetry
over Server-Sent Events, and serves the static dashboard in ``web/``.

Endpoints
---------
GET  /api/health     -> {"status": "ok", "source": "sim"|"aws-iot"}
GET  /api/pid        -> current PID gains
POST /api/pid        -> validate + apply a (partial) gain update; 400 on bad input
GET  /api/telemetry  -> latest telemetry sample
GET  /api/history    -> recent telemetry samples (for first paint)
GET  /api/stream     -> text/event-stream of live telemetry (SSE)
GET  /<file>         -> static assets from web/ (index.html at /)

Run::

    python -m api.server                 # simulator (no hardware needed)
    python -m api.server --source aws ...  # live, via AWS IoT (see iot.py)
"""
from __future__ import annotations

import argparse
import json
import os
import time
from functools import partial
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

from api.telemetry import GainError, RobotState, SimSource, validate_gains

WEB_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "web")

_CONTENT_TYPES = {
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".json": "application/json",
    ".svg": "image/svg+xml",
    ".ico": "image/x-icon",
    ".png": "image/png",
}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def __init__(self, state: RobotState, web_dir: str, source, *args, **kwargs) -> None:
        self.state = state
        self.web_dir = web_dir
        self.source = source
        super().__init__(*args, **kwargs)

    # ----- routing -----------------------------------------------------------
    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/api/health":
            self._json({"status": "ok", "source": getattr(self.source, "label", "sim")})
        elif path == "/api/pid":
            self._json(self.state.get_gains())
        elif path == "/api/telemetry":
            latest = self.state.latest()
            self._json(latest.to_dict() if latest else {})
        elif path == "/api/history":
            self._json([t.to_dict() for t in self.state.history()])
        elif path == "/api/stream":
            self._stream()
        else:
            self._static(path)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        if path != "/api/pid":
            self._json({"error": "not found"}, status=404)
            return
        length = int(self.headers.get("Content-Length", 0) or 0)
        raw = self.rfile.read(length) if length else b""
        try:
            payload = json.loads(raw.decode("utf-8") or "{}")
            clean = validate_gains(payload)
        except (ValueError, GainError) as exc:
            self._json({"error": str(exc)}, status=400)
            return
        new_gains = self.state.update_gains(clean)
        publish = getattr(self.source, "publish_gains", None)
        if callable(publish):
            try:
                publish(new_gains)
            except Exception:
                pass  # dashboard still reflects the change locally
        self._json(new_gains)

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self._cors()
        self.send_header("Content-Length", "0")
        self.end_headers()

    # ----- helpers -----------------------------------------------------------
    def _json(self, obj, status: int = 200) -> None:
        body = json.dumps(obj).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self._cors()
        self.end_headers()
        self.wfile.write(body)

    def _cors(self) -> None:
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def _stream(self) -> None:
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self._cors()
        self.end_headers()
        try:
            while True:
                latest = self.state.latest()
                if latest is not None:
                    msg = f"data: {json.dumps(latest.to_dict())}\n\n"
                    self.wfile.write(msg.encode("utf-8"))
                    self.wfile.flush()
                time.sleep(0.1)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            return  # client navigated away; end this stream thread

    def _static(self, path: str) -> None:
        rel = "index.html" if path == "/" else path.lstrip("/")
        full = os.path.normpath(os.path.join(self.web_dir, rel))
        if not full.startswith(os.path.normpath(self.web_dir)):
            self._json({"error": "forbidden"}, status=403)
            return
        if not os.path.isfile(full):
            self._json({"error": "not found"}, status=404)
            return
        ext = os.path.splitext(full)[1].lower()
        with open(full, "rb") as fh:
            body = fh.read()
        self.send_response(200)
        self.send_header("Content-Type", _CONTENT_TYPES.get(ext, "application/octet-stream"))
        self.send_header("Content-Length", str(len(body)))
        self._cors()
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args) -> None:  # keep the console quiet
        pass


def make_server(host: str, port: int, state: RobotState, web_dir: str, source) -> ThreadingHTTPServer:
    server = ThreadingHTTPServer((host, port), partial(Handler, state, web_dir, source))
    server.daemon_threads = True
    return server


def _build_source(args, state: RobotState):
    if args.source == "aws":
        from api.iot import AwsIotSource  # imported lazily so paho stays optional
        missing = [n for n in ("endpoint", "ca", "cert", "key") if not getattr(args, n)]
        if missing:
            raise SystemExit(f"--source aws requires: {', '.join('--' + m for m in missing)}")
        return AwsIotSource(state, args.endpoint, args.ca, args.cert, args.key)
    return SimSource(state)


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="BalanceBot companion API + dashboard")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--source", choices=("sim", "aws"), default="sim",
                        help="telemetry source (default: built-in simulator)")
    parser.add_argument("--endpoint", help="AWS IoT Core ATS endpoint (for --source aws)")
    parser.add_argument("--ca", help="root CA cert path")
    parser.add_argument("--cert", help="device cert path")
    parser.add_argument("--key", help="device private key path")
    args = parser.parse_args(argv)

    state = RobotState()
    source = _build_source(args, state)
    source.start()

    server = make_server(args.host, args.port, state, WEB_DIR, source)
    print(f"BalanceBot companion → http://{args.host}:{args.port}  (source: {source.label})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nshutting down…")
    finally:
        server.shutdown()
        server.server_close()
        stop = getattr(source, "stop", None)
        if callable(stop):
            stop()


if __name__ == "__main__":
    main()
