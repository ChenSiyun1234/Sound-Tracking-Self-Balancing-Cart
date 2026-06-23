"""Integration tests for the companion API.

Starts the real stdlib server (on an ephemeral port) with the simulator as the
data source, then exercises the REST surface over HTTP. Run from the
``companion-app`` directory::

    python -m unittest api.tests.test_api
"""
from __future__ import annotations

import json
import os
import sys
import threading
import time
import unittest
import urllib.error
import urllib.request

# Allow running both as `python -m unittest api.tests.test_api` (from companion-app)
# and directly: make sure companion-app is importable.
_COMPANION_APP = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if _COMPANION_APP not in sys.path:
    sys.path.insert(0, _COMPANION_APP)

from api.server import WEB_DIR, make_server  # noqa: E402
from api.telemetry import DEFAULT_GAINS, RobotState, SimSource  # noqa: E402


class CompanionApiTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.state = RobotState()
        cls.sim = SimSource(cls.state)
        cls.sim.start()
        cls.server = make_server("127.0.0.1", 0, cls.state, WEB_DIR, cls.sim)
        cls.port = cls.server.server_address[1]
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls._wait_until_healthy()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.server.shutdown()
        cls.server.server_close()
        cls.sim.stop()

    @classmethod
    def _wait_until_healthy(cls) -> None:
        for _ in range(50):
            try:
                status, _ = cls._raw_request("GET", "/api/health")
                if status == 200:
                    return
            except OSError:
                pass
            time.sleep(0.05)
        raise RuntimeError("server did not become healthy in time")

    @classmethod
    def _raw_request(cls, method: str, path: str, body=None):
        url = f"http://127.0.0.1:{cls.port}{path}"
        data = json.dumps(body).encode("utf-8") if body is not None else None
        req = urllib.request.Request(url, data=data, method=method)
        if data is not None:
            req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req, timeout=5) as resp:
                return resp.status, resp.read().decode("utf-8")
        except urllib.error.HTTPError as exc:
            return exc.code, exc.read().decode("utf-8")

    def request(self, method: str, path: str, body=None):
        return self._raw_request(method, path, body)

    # ----- tests -------------------------------------------------------------
    def test_health(self) -> None:
        status, text = self.request("GET", "/api/health")
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(text)["status"], "ok")
        self.assertEqual(json.loads(text)["source"], "sim")

    def test_get_pid_returns_all_gains(self) -> None:
        status, text = self.request("GET", "/api/pid")
        self.assertEqual(status, 200)
        self.assertEqual(set(json.loads(text)), set(DEFAULT_GAINS))

    def test_post_pid_partial_update(self) -> None:
        status, text = self.request("POST", "/api/pid", {"angle_kp": 22.5})
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(text)["angle_kp"], 22.5)
        # change is persisted
        _, text2 = self.request("GET", "/api/pid")
        self.assertEqual(json.loads(text2)["angle_kp"], 22.5)

    def test_post_pid_rejects_unknown_key(self) -> None:
        status, text = self.request("POST", "/api/pid", {"nope": 1.0})
        self.assertEqual(status, 400)
        self.assertIn("unknown gain", json.loads(text)["error"])

    def test_post_pid_rejects_non_numeric(self) -> None:
        status, _ = self.request("POST", "/api/pid", {"angle_kp": "abc"})
        self.assertEqual(status, 400)

    def test_post_pid_rejects_out_of_range(self) -> None:
        status, _ = self.request("POST", "/api/pid", {"angle_kp": -5})
        self.assertEqual(status, 400)

    def test_telemetry_is_streaming(self) -> None:
        time.sleep(0.2)  # let the simulator produce at least one sample
        status, text = self.request("GET", "/api/telemetry")
        self.assertEqual(status, 200)
        sample = json.loads(text)
        self.assertIn("pitch_deg", sample)
        self.assertIsInstance(sample["pitch_deg"], (int, float))
        self.assertEqual(sample["source"], "sim")

    def test_static_dashboard_served(self) -> None:
        status, text = self.request("GET", "/")
        self.assertEqual(status, 200)
        self.assertIn("<html", text.lower())
        self.assertIn("BalanceBot", text)


if __name__ == "__main__":
    unittest.main()
