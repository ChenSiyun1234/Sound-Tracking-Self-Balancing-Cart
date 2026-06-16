# Self-Balancing Two-Wheel Robot

Bare-metal embedded-C firmware for a two-wheel **inverted-pendulum robot** that balances itself, drives over Bluetooth, holds its position when pushed, and scans-and-turns around obstacles. Built on two MCU platforms. *(UC Davis EEC 172 final project.)*

![Language](https://img.shields.io/badge/Embedded-C-blue)
![MCU](https://img.shields.io/badge/MCU-STM32F103%20%C2%B7%20CC3200-green)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-orange)
![Control](https://img.shields.io/badge/Control-Cascaded%20PID-red)
![Status](https://img.shields.io/badge/status-running%20on%20hardware-brightgreen)

A two-wheel cart keeps itself upright with a **cascaded three-loop PID controller** (angle + velocity + heading) running at **100 Hz inside the IMU's data-ready interrupt**, fed by **on-chip DMP sensor fusion**. It is remote-controlled over Bluetooth, returns to its start point when pushed, sweeps a fixed ultrasonic sensor to find the clearest heading around obstacles, tunes its own gains live, and persists them to internal flash.

---

## What it demonstrates

- **Real-time control systems** — cascaded angle-PD / velocity-PI / heading-PD control of an inverted pendulum, with integral anti-windup, **motor dead-zone compensation** (kills the limit-cycle wobble), output saturation, and fall protection.
- **Bare-metal + RTOS firmware** — register-level peripheral drivers (StdPeriph) under **FreeRTOS**; the safety-critical balance loop is **hardware-paced in an ISR**, fully decoupled from the task scheduler.
- **Sensor fusion** — MPU-6050 gyro/accel fused by the on-chip **DMP** (quaternion → pitch/yaw) over a software-I²C bus, driven by a data-ready interrupt.
- **Embedded systems debugging** — diagnosed and fixed a velocity-loop **positive-feedback runaway** (live sign-toggle), a motor **dead-zone limit cycle**, an int16 **PWM-mix overflow**, and added **read-back-verified** flash writes.
- **Hardware integration** — TB6612 H-bridge PWM, quadrature encoders, HC-05 Bluetooth (SPP), HC-SR04 ultrasonic (DWT-cycle-counter timing), SSD1306 OLED, and a 5 V-tolerant pin remap so 5 V encoder signals reach a 3.3 V MCU safely.
- **IoT / networking** *(CC3200 build)* — Wi-Fi + **AWS IoT over TLS** (Thing Shadow → IoT Rule → SNS) emails an alert on power-up and on a fall.

---

## Two implementations

| Build | MCU | Highlights | Status |
|-------|-----|-----------|--------|
| [**STM32**](STM32/README.md) — *flagship* | STM32F103C8T6 "Blue Pill" (Cortex-M3, 72 MHz) | Bluetooth remote, ultrasonic scan-and-turn avoidance, **live PID tuning**, flash-persisted config, dual OLED views | ✅ Balancing, remote & tuning verified on hardware |
| [**CC3200**](CC3200/README.md) | TI CC3200 (Cortex-M4 + Wi-Fi) | Kalman sensor fusion, colour OLED, **Wi-Fi / AWS-IoT** email alerts | ✅ Working |

Both run the same control law on the same TARKBOT R3T chassis; they differ in MCU, sensor-fusion method (DMP vs. Kalman), and feature focus (motion/UX vs. networking).

### ▶ STM32 build — *flagship*

Bluetooth remote · ultrasonic scan-and-turn avoidance · live PID tuning · flash-persisted config. **[Full details & build →](STM32/README.md)**

| Self-balancing + remote | OLED telemetry |
|:---:|:---:|
| <img src="STM32/images/stm32_balancing.gif" width="220" alt="STM32 cart self-balancing"> | <img src="STM32/images/stm32_oled.jpg" width="280" alt="STM32 OLED status"> |

<img src="STM32/images/wiring_stm32.png" width="820" alt="STM32 wiring diagram">

### ▶ CC3200 build

Kalman sensor fusion · colour OLED · **Wi-Fi / AWS-IoT** email alerts on power-up and fall. **[Full details & build →](CC3200/README.md)**

| Self-balancing | OLED telemetry |
|:---:|:---:|
| <img src="CC3200/images/cc3200_balancing.gif" width="220" alt="CC3200 cart self-balancing"> | <img src="CC3200/images/cc3200_oled.gif" width="220" alt="CC3200 OLED status"> |

<img src="CC3200/images/wiring_cc3200.png" width="820" alt="CC3200 wiring diagram">

---

## Control architecture

```
  MPU-6050 ──DMP──▶ pitch θ, rate ─────────▶ angle PD ───┐
  (accel+gyro)                                           │
                                                         ├─▶ Σ ─▶ dead-zone comp ─▶ ±3600 PWM ─▶ TB6612 ─▶ wheels
  encoders ──────▶ wheel speed ────────────▶ velocity PI ┤        (anti-overflow,
                                                         │         output clamp)
  gyro (yaw) ─────────────────────────────▶ heading PD ──┘
                                                              ▲
        the whole loop runs at 100 Hz inside the IMU ISR ─────┘
```

The **angle loop** keeps it upright; the **velocity loop** stops it and drives it back to where it started when pushed; the **heading loop** steers and powers the in-place rotation used to scan for obstacles. Remote and avoidance only set the `vx`/`vw` targets — the balance loop is never interrupted.

---

## Tech stack

`C` · STM32F103 StdPeriph · FreeRTOS V9 · Keil µVision 5 / Arm Compiler 5 · MPU-6050 DMP · TB6612FNG · HC-05 · HC-SR04 · SSD1306
*(CC3200 build: TI SimpleLink SDK · AWS IoT · TLS · Code Composer Studio)*

---

## Repository

```
self-balancing-cart/
├── STM32/      flagship firmware — Keil project + WIRING.md + TUNING.md
├── CC3200/     TI CCS firmware — Wi-Fi / AWS-IoT build
└── README.md   (this file)
```

→ **[STM32 build & features](STM32/README.md)** · **[Wiring](STM32/WIRING.md)** · **[PID tuning guide](STM32/TUNING.md)** · **[CC3200 build](CC3200/README.md)**

---

## Acknowledgements

Control structure and baseline gains adapted from the **XTARK / TARKBOT R3T** balancing firmware; CC3200 OLED driver from **Adafruit GFX**; networking from the **TI SimpleLink** examples.
