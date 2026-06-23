"""Live telemetry from the real robot via AWS IoT Core (MQTT over TLS).

This mirrors the CC3200 firmware path documented in the repo README: the robot
publishes to AWS IoT Core (Thing Shadow / a telemetry topic); here we subscribe,
parse each message into a :class:`~api.telemetry.Telemetry`, and feed the shared
``RobotState``. PID updates from the dashboard are published back to a command
topic so the robot can adopt them.

``paho-mqtt`` is an *optional* dependency — the app defaults to the built-in
simulator so it runs with no cloud setup. Enable this path with::

    pip install paho-mqtt
    python -m api.server --source aws --endpoint <id>-ats.iot.<region>.amazonaws.com \\
        --ca AmazonRootCA1.pem --cert device.pem.crt --key private.pem.key
"""
from __future__ import annotations

import json
import threading
import time

from api.telemetry import RobotState, Telemetry

TELEMETRY_TOPIC = "balancebot/telemetry"
COMMAND_TOPIC = "balancebot/cmd/pid"


class AwsIotSource:
    """Subscribes to the robot's AWS IoT telemetry topic and publishes gain commands."""

    label = "aws-iot"

    def __init__(
        self,
        state: RobotState,
        endpoint: str,
        ca: str,
        cert: str,
        key: str,
        client_id: str = "balancebot-companion",
        port: int = 8883,
    ) -> None:
        try:
            import paho.mqtt.client as mqtt
        except ImportError as exc:  # keep the app usable without the optional dep
            raise RuntimeError(
                "paho-mqtt is not installed. Run `pip install paho-mqtt`, or drop "
                "`--source aws` to use the built-in simulator."
            ) from exc

        self.state = state
        self.endpoint = endpoint
        self.port = port
        self.client = mqtt.Client(client_id=client_id)
        self.client.tls_set(ca_certs=ca, certfile=cert, keyfile=key)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc) -> None:
        client.subscribe(TELEMETRY_TOPIC)

    def _on_message(self, client, userdata, msg) -> None:
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return
        self.state.push(Telemetry(
            ts=float(payload.get("ts", time.time())),
            pitch_deg=float(payload.get("pitch_deg", 0.0)),
            velocity=float(payload.get("velocity", 0.0)),
            heading_deg=float(payload.get("heading_deg", 0.0)),
            pid_output=float(payload.get("pid_output", 0.0)),
            upright=bool(payload.get("upright", True)),
            source=self.label,
        ))

    def publish_gains(self, gains: dict[str, float]) -> None:
        """Push new PID gains down to the robot over the command topic."""
        self.client.publish(COMMAND_TOPIC, json.dumps(gains), qos=1)

    def start(self) -> None:
        self.client.connect(self.endpoint, self.port, keepalive=60)
        threading.Thread(
            target=self.client.loop_forever, daemon=True, name="balancebot-mqtt"
        ).start()

    def stop(self) -> None:
        try:
            self.client.disconnect()
        except Exception:
            pass
