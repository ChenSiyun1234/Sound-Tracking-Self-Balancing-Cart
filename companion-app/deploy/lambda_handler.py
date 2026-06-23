"""AWS Lambda handler backing the serverless deployment (see template.yaml).

Routes:
  GET  /api/pid              -> current gains (from DynamoDB, falling back to defaults)
  POST /api/pid              -> validate + persist new gains
  (IoT TopicRule invocation) -> store the latest telemetry sample

This is the cloud-deploy counterpart to the local dev server in ``api/server.py``.
It is intentionally small; ``boto3`` is provided by the Lambda runtime, and the
handler stays importable without it so the module can be unit-tested locally.
"""
import json
import os

try:
    import boto3
    _table = boto3.resource("dynamodb").Table(os.environ["GAINS_TABLE"])
except Exception:  # boto3/table absent locally — handler still imports
    _table = None

GAIN_KEYS = ("angle_kp", "angle_kd", "vel_kp", "vel_ki", "heading_kp", "heading_kd")
DEFAULTS = {
    "angle_kp": 14.0, "angle_kd": 0.9, "vel_kp": 0.06,
    "vel_ki": 0.001, "heading_kp": 1.2, "heading_kd": 0.05,
}


def _validate(payload):
    if not isinstance(payload, dict):
        raise ValueError("PID payload must be a JSON object")
    clean = {}
    for key, value in payload.items():
        if key not in DEFAULTS:
            raise ValueError(f"unknown gain '{key}'")
        num = float(value)
        if not (0 <= num <= 1000):
            raise ValueError(f"gain '{key}' out of range")
        clean[key] = num
    if not clean:
        raise ValueError("no valid gains supplied")
    return clean


def _response(status, body):
    return {
        "statusCode": status,
        "headers": {"Content-Type": "application/json"},
        "body": json.dumps(body),
    }


def handler(event, context):
    method = event.get("requestContext", {}).get("http", {}).get("method")

    # No HTTP method => invoked by the IoT TopicRule with a telemetry payload.
    if method is None:
        if _table is not None:
            _table.put_item(Item={"id": "telemetry", "payload": json.dumps(event)})
        return {"ok": True}

    if method == "GET":
        gains = dict(DEFAULTS)
        if _table is not None:
            item = (_table.get_item(Key={"id": "gains"}).get("Item")) or {}
            for key in GAIN_KEYS:
                if key in item:
                    gains[key] = float(item[key])
        return _response(200, gains)

    if method == "POST":
        try:
            clean = _validate(json.loads(event.get("body") or "{}"))
        except (ValueError, TypeError) as exc:
            return _response(400, {"error": str(exc)})
        merged = {**DEFAULTS, **clean}
        if _table is not None:
            # NOTE: production code would store these as Decimal, not float.
            _table.put_item(Item={"id": "gains", **{k: str(v) for k, v in merged.items()}})
        return _response(200, merged)

    return _response(405, {"error": "method not allowed"})
