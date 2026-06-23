"""BalanceBot companion API package.

A small, dependency-free Python service that bridges the self-balancing robot
to a web dashboard: it serves live telemetry over Server-Sent Events and accepts
PID-gain updates over a REST endpoint. The default data source is a hardware-free
simulator (telemetry.SimSource); the live source is AWS IoT Core (iot.AwsIotSource).
"""
