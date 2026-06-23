"""Telemetry model, shared robot state, and a hardware-free simulator.

The real robot (CC3200 build) publishes its state to AWS IoT Core; see ``iot.py``
for the live path. ``SimSource`` lets the entire app run with zero hardware and
zero cloud credentials, so the dashboard and API can be developed and demoed
offline. Both feed the same ``RobotState``.
"""
from __future__ import annotations

import math
import random
import threading
import time
from collections import deque
from dataclasses import asdict, dataclass

# Default cascaded-PID gains, mirroring the firmware's starting values.
DEFAULT_GAINS: dict[str, float] = {
    "angle_kp": 14.0,
    "angle_kd": 0.9,
    "vel_kp": 0.06,
    "vel_ki": 0.001,
    "heading_kp": 1.2,
    "heading_kd": 0.05,
}

GAIN_KEYS: tuple[str, ...] = tuple(DEFAULT_GAINS)


class GainError(ValueError):
    """Raised when a PID-gain payload is malformed or out of range."""


def validate_gains(payload: object) -> dict[str, float]:
    """Validate a (possibly partial) PID-gain update.

    Returns a clean ``{gain: float}`` dict containing only the supplied keys, or
    raises :class:`GainError`. Callers merge the result onto the current gains,
    so a partial update (e.g. just ``angle_kp``) is allowed.
    """
    if not isinstance(payload, dict):
        raise GainError("PID payload must be a JSON object")
    clean: dict[str, float] = {}
    for key, value in payload.items():
        if key not in DEFAULT_GAINS:
            raise GainError(f"unknown gain '{key}'")
        try:
            num = float(value)
        except (TypeError, ValueError):
            raise GainError(f"gain '{key}' must be a number")
        if not math.isfinite(num) or num < 0 or num > 1000:
            raise GainError(f"gain '{key}' out of range [0, 1000]")
        clean[key] = num
    if not clean:
        raise GainError("no valid gains supplied")
    return clean


@dataclass
class Telemetry:
    """A single telemetry sample from the robot (or simulator)."""

    ts: float
    pitch_deg: float       # body tilt; the balance loop drives this toward 0
    velocity: float        # forward-velocity estimate (body frame)
    heading_deg: float
    pid_output: float      # motor command after the cascaded loop, [-100, 100]
    upright: bool
    source: str            # "sim" or "aws-iot"

    def to_dict(self) -> dict:
        return asdict(self)


class RobotState:
    """Thread-safe holder for the latest telemetry, gains, and a short history."""

    def __init__(self, history: int = 240) -> None:
        self._lock = threading.Lock()
        self._gains = dict(DEFAULT_GAINS)
        self._latest: Telemetry | None = None
        self._history: deque[Telemetry] = deque(maxlen=history)

    def get_gains(self) -> dict[str, float]:
        with self._lock:
            return dict(self._gains)

    def update_gains(self, partial: dict[str, float]) -> dict[str, float]:
        with self._lock:
            self._gains.update(partial)
            return dict(self._gains)

    def push(self, telemetry: Telemetry) -> None:
        with self._lock:
            self._latest = telemetry
            self._history.append(telemetry)

    def latest(self) -> Telemetry | None:
        with self._lock:
            return self._latest

    def history(self) -> list[Telemetry]:
        with self._lock:
            return list(self._history)


class SimSource(threading.Thread):
    """Fakes an inverted pendulum that reacts to the live PID gains.

    The model is deliberately simple: a pendulum that gravity tips over and a
    PD balance loop fights back. Turn ``angle_kp`` / ``angle_kd`` up and the cart
    settles near upright; turn them down and it diverges, "falls" (``upright``
    goes False) and auto-recovers — so live tuning is visible on the dashboard.
    """

    label = "sim"

    def __init__(self, state: RobotState, rate_hz: float = 20.0) -> None:
        super().__init__(daemon=True, name="balancebot-sim")
        self.state = state
        self.dt = 1.0 / rate_hz
        self._stop = threading.Event()
        self.theta = 2.0   # deg, slight initial lean
        self.omega = 0.0   # deg/s
        self.velocity = 0.0
        self.heading = 0.0

    def stop(self) -> None:
        self._stop.set()

    def run(self) -> None:
        gravity_gain = 320.0  # how hard gravity tips the pendulum over
        while not self._stop.is_set():
            gains = self.state.get_gains()
            gravity = gravity_gain * math.sin(math.radians(self.theta))
            control = gains["angle_kp"] * self.theta + gains["angle_kd"] * self.omega
            accel = gravity - control + random.uniform(-3.0, 3.0)
            self.omega = max(-3000.0, min(3000.0, self.omega + accel * self.dt))
            self.theta += self.omega * self.dt

            self.velocity = 0.9 * self.velocity - 0.1 * self.theta \
                + random.uniform(-0.05, 0.05)
            self.heading = (self.heading + gains["heading_kp"] * 0.1
                            + random.uniform(-0.5, 0.5)) % 360.0

            upright = abs(self.theta) < 30.0
            pid_output = max(-100.0, min(100.0, control))
            if not upright:
                # A real fall cuts the motors; the sim re-rights itself for the demo.
                self.theta = random.uniform(-3.0, 3.0)
                self.omega = 0.0
                self.velocity = 0.0

            self.state.push(Telemetry(
                ts=time.time(),
                pitch_deg=round(self.theta, 3),
                velocity=round(self.velocity, 3),
                heading_deg=round(self.heading, 2),
                pid_output=round(pid_output, 2),
                upright=upright,
                source=self.label,
            ))
            time.sleep(self.dt)
