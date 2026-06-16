//*****************************************************************************
// hw_config.h
//
// Central definitions for pins, timers, addresses and tuning constants.
//   PIN_xx  = CC3200 device pin used by the SDK pin-mux (MAP_PinType*).
//   GPIOn   = GPIO number 0..31 used by the common gpio_if helper.
//   Px.y    = physical LaunchPad header position.
//*****************************************************************************
#ifndef __HW_CONFIG_H__
#define __HW_CONFIG_H__

#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "pin.h"

//=============================================================================
// 0. Build options
//=============================================================================
#define ENABLE_OLED          1   // SSD1351 128x128 colour OLED (4-wire SPI; pins in pinmux.c)
#define ENABLE_DEBUG_UART    1   // UART0 console over the USB FTDI (115200 8N1)

// Motor PWM wiring:
//   1 = SHARED PWM: PIN_64/P3.9 feeds both TB6612 PWMA and PWMB (one shared
//       duty, no per-wheel control). Keeps the JTAG TDO pin free.
//   0 = INDEPENDENT PWM: left = PIN_64/P3.9, right = PIN_17/GPIO24. PIN_17 must
//       be reached via JTAG header J19 (=TDO) or a 0-Ohm on P4.3.
#define MOTOR_SHARED_PWM     1

// When 1, the console prints raw IMU axes ~4x/s (accel in mg, gyro in deg/s).
#define DEBUG_IMU_RAW        0

//   DEBUG_VERBOSE      : print boot self-checks and a per-tick state line.
//   MOTOR_TEST_ON_BOOT : drive the motors through a fixed sequence at boot
//                        (put the cart on a stand, wheels off the ground).
#define DEBUG_VERBOSE        1
#define MOTOR_TEST_ON_BOOT   0

// Wi-Fi email alerts via AWS IoT (cart_email.c). Enabling also requires CCS
// build changes: add the SimpleLink include paths + simplelink.a and remove
// the NON_NETWORK symbol.
#define ENABLE_WIFI_EMAIL    1

// Accel-only mode: run without the gyro (gyro rate treated as 0).
#define ALLOW_NO_GYRO        0

//=============================================================================
// 1. I2C bus  (PIN_01 = SCL / P1.9,  PIN_02 = SDA / P1.10) - shared by:
//      - BMA222 on-board accelerometer (addr 0x18, defined in bma222drv.h)
//      - MPU-6050 IMU                  (addr 0x68)
//    Board has 2.2 kOhm pull-ups on these lines. (The SSD1351 OLED is on SPI,
//    not this bus - see pinmux.c.)
//=============================================================================
#define I2C_SCL_PIN          PIN_01
#define I2C_SDA_PIN          PIN_02
#define I2C_PIN_MODE         PIN_MODE_1

#define MPU6050_I2C_ADDR     0x68

//=============================================================================
// 2. Balance sensing  (accelerometer source + axis mapping)
//    AXIS index: 0=X, 1=Y, 2=Z.  SIGN: +1/-1.
//=============================================================================
// 1 = CC3200 on-board BMA222 for the accel angle
// 0 = MPU-6050's own accelerometer (co-aligns accel+gyro axes; angle uses a
//     ratio via atan2, so accel scale does not matter)
#define ACCEL_SOURCE_IS_BMA222   0

// BMA222 axis -> tilt geometry (used when ACCEL_SOURCE_IS_BMA222 == 1)
#define BMA_FORWARD_AXIS     0    // axis pointing in the cart's travel direction
#define BMA_FORWARD_SIGN     1
#define BMA_VERTICAL_AXIS    2    // axis pointing up when upright
#define BMA_VERTICAL_SIGN    1

// MPU-6050 accel axis -> tilt geometry (active when ACCEL_SOURCE_IS_BMA222 == 0).
// Rule: lean forward -> both wheels forward. If it reverses, flip both this and
// MPU_GYRO_PITCH_SIGN together.
#define MPU_FORWARD_AXIS     1    // Y (forward/travel direction)
#define MPU_FORWARD_SIGN     1
#define MPU_VERTICAL_AXIS    2    // Z (up when upright)
#define MPU_VERTICAL_SIGN    1

// MPU-6050 gyro axis measuring PITCH rate. Cart pitches about the X axis
// (X = wheel-axle), so PITCH = axis 0. Used by the angle-loop D term and Kalman.
#define MPU_GYRO_PITCH_AXIS  0
// Flip together with MPU_FORWARD_SIGN if the drive direction reverses.
#define MPU_GYRO_PITCH_SIGN  1

// MPU-6050 gyro axis measuring YAW rate (turning), for the turn loop.
#define MPU_GYRO_YAW_AXIS    2
#define MPU_GYRO_YAW_SIGN    1

//=============================================================================
// 3. Kalman filter tuning  (increase R to trust the gyro more / accel less;
//    increase Q_bias if the gyro drifts).
//=============================================================================
#define KALMAN_Q_ANGLE       0.001f
#define KALMAN_Q_BIAS        0.003f
#define KALMAN_R_MEASURE     0.030f

//=============================================================================
// 4. Balance control - cascaded loops (angle PD + velocity PI + turn PD).
//    Tune live over serial - see the key table in main.c.
//=============================================================================
#define BALANCE_LOOP_DT_S    0.005f   // control period = 5 ms (200 Hz)

// --- Angle loop (PD on tilt + gyro rate).
//     Tune: q/a = Kp +/-1   w/s = Kd +/-0.1   e/d = Ki +/-0.02
#define BALANCE_KP           13.0f
#define BALANCE_KI           0.0f     // angle loop runs as PD; keep Ki 0
#define BALANCE_KD           0.7f
#define BALANCE_I_CLAMP      200.0f
#define BALANCE_SAFE_TILT_DEG 35.0f   // beyond this -> Fault-Stop

// --- Velocity loop (PI on wheel speed). Needs ENCODER_ENABLED=1 (section 11).
//     Tune: r/f=VKp  c/v=VKi
#define USE_VELOCITY_LOOP    0
#define VELOCITY_KP          566.0f   // PWM per (m/s)
#define VELOCITY_KI          283.0f   // PWM per (m of displacement)
#define VELOCITY_INT_CLAMP   1.5f     // |displacement| limit (m)
#define VELOCITY_LPF_ALPHA   0.4f     // new-sample weight in the speed low-pass

// --- Turn loop (PD on yaw rate). Optional.
#define USE_TURN_LOOP        0
#define TURN_KP              0.07f
#define TURN_KD              0.35f

//=============================================================================
// 5. Motor PWM carrier  (~19.6 kHz).  256 duty levels [0..255].
//    period = GRANULARITY*255 ticks @ 80 MHz -> 80e6/4080 = 19.6 kHz.
//=============================================================================
#define MOTOR_PWM_GRANULARITY  16
#define MOTOR_PWM_RELOAD       (MOTOR_PWM_GRANULARITY * 255)   // 4080

//=============================================================================
// 6. Motor LEFT (TB6612 channel A)  -> drives the cart's LEFT wheel
//    PWMA = PIN_64 / GPIO09 / P3.9  = GT_PWM05 (TIMERA2 Timer B, mode 3) - CLEAN
//=============================================================================
#define MOTOR_L_PWM_PIN      PIN_64
#define MOTOR_L_PWM_PIN_MODE PIN_MODE_3
#define MOTOR_L_PWM_PERIPH   PRCM_TIMERA2
#define MOTOR_L_PWM_BASE     TIMERA2_BASE
#define MOTOR_L_PWM_TIMER    TIMER_B
#define MOTOR_L_PWM_CFG      (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PWM)

#define MOTOR_AIN1_GPIO      8        // PIN_63 / P3.7
#define MOTOR_AIN1_PIN       PIN_63
#define MOTOR_AIN2_GPIO      30       // PIN_53 / P3.8
#define MOTOR_AIN2_PIN       PIN_53
#define MOTOR_L_INVERT       0        // flip if the LEFT wheel runs backward

//=============================================================================
// 7. Motor RIGHT (TB6612 channel B) -> drives the cart's RIGHT wheel
//    PWMB = PIN_17 / GPIO24         = GT_PWM00 (TIMERA0 Timer A, mode 5)
//    NOTE: P4.3 not connected by default. Reach GPIO24 by populating the P4.3
//          0-Ohm link or jumpering from JTAG header J19 (=TDO).
//=============================================================================
#if MOTOR_SHARED_PWM
// Right channel routes to the same timer/pin as the left; PIN_17 stays unmuxed.
#define MOTOR_R_PWM_PIN      PIN_64
#define MOTOR_R_PWM_PIN_MODE PIN_MODE_3
#define MOTOR_R_PWM_PERIPH   PRCM_TIMERA2
#define MOTOR_R_PWM_BASE     TIMERA2_BASE
#define MOTOR_R_PWM_TIMER    TIMER_B
#define MOTOR_R_PWM_CFG      (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PWM)
#else
#define MOTOR_R_PWM_PIN      PIN_17
#define MOTOR_R_PWM_PIN_MODE PIN_MODE_5
#define MOTOR_R_PWM_PERIPH   PRCM_TIMERA0
#define MOTOR_R_PWM_BASE     TIMERA0_BASE
#define MOTOR_R_PWM_TIMER    TIMER_A
#define MOTOR_R_PWM_CFG      (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM)
#endif

#define MOTOR_BIN1_GPIO      0        // PIN_50 / P3.10
#define MOTOR_BIN1_PIN       PIN_50
#define MOTOR_BIN2_GPIO      31       // PIN_45 / P2.4
#define MOTOR_BIN2_PIN       PIN_45
// Both motors use the same invert so one command spins both wheels the same way.
#define MOTOR_R_INVERT       0

//=============================================================================
// 8. Motor driver STBY (shared enable; HIGH = run, LOW = coast/disable)
//    GPIO28 / PIN_18 / P2.2
//=============================================================================
// STBY mode:
//   0 = TB6612 STBY tied to 3.3V externally; PIN_18 free for the OLED reset.
//   1 = drive STBY from the GPIO below.
#define MOTOR_STBY_USE_GPIO  0
#define MOTOR_STBY_GPIO      28
#define MOTOR_STBY_PIN       PIN_18   // NOTE: also the OLED RST pin when OLED is on

//=============================================================================
// 9. Microsecond time-base  (TIMERA1, full 32-bit)  - used for dt + scheduling
//=============================================================================
#define CLOCK_TIMER_PERIPH   PRCM_TIMERA1
#define CLOCK_TIMER_BASE     TIMERA1_BASE

//=============================================================================
// 10. Debug console UART0 over USB (PIN_55 = TX / P1.4, PIN_57 = RX / P1.3)
//=============================================================================
#define DBG_UART_TX_PIN      PIN_55
#define DBG_UART_RX_PIN      PIN_57
#define DBG_UART_PIN_MODE    PIN_MODE_3

//=============================================================================
// 11. Wheel encoders  (needed by the velocity loop; off for angle-only build).
//     TARKBOT MC130 encoder TT motors: 13-line Hall AB quadrature, 1:48 gear.
//     Single-channel rising-edge count: 13 x 48 = 624 counts/wheel-rev (x1).
//
//     CC3200 is 3.3 V and NOT 5 V-tolerant. Power the encoder from 3.3 V so the
//     A/B signals are 3.3 V, or the GPIO will be damaged.
//     MC130 6-pin: 1=MotorM 2=EncGND 3=EncA 4=EncB 5=EncVCC(->3V3) 6=MotorP.
//=============================================================================
#define ENCODER_ENABLED        0      // set 1 (and USE_VELOCITY_LOOP) when wired
#define ENCODER_USE_QUADRATURE 0
// PIN_61/PIN_07 are taken by the SSD1351 OLED (DC / MOSI), so single-channel
// inputs use PIN_08 (left) and PIN_06 (right).
#define ENCODER_L_A_GPIO     17
#define ENCODER_L_A_PIN      PIN_08
#define ENCODER_R_A_GPIO     15
#define ENCODER_R_A_PIN      PIN_06
#define ENCODER_L_B_GPIO     22       // only used if ENCODER_USE_QUADRATURE=1
#define ENCODER_L_B_PIN      PIN_15
#define ENCODER_R_B_GPIO     13
#define ENCODER_R_B_PIN      PIN_04

// Physical constants (TARKBOT R3T chassis + MC130 1:48, Ø66 mm wheel).
#define WHEEL_DIAMETER_M        0.066f
#define WHEELBASE_M             0.167f          // distance between the two wheels
#define ENCODER_COUNTS_PER_REV  624.0f          // 13 lines x 48 gear (x1 counting)
#define WHEEL_M_PER_COUNT       (3.14159265f * WHEEL_DIAMETER_M / ENCODER_COUNTS_PER_REV)

//=============================================================================
// 12. Optional modules kept in ../../BalanceCart_CC3200_optional/ (out of the
//     CCS root so they are not auto-compiled). Pins below are suggestions from
//     the free list.
//=============================================================================
#define BUZZER_GPIO          6        // PIN_61 / P1.5  (active buzzer)
#define BUZZER_PIN           PIN_61
#define BUZZER_ACTIVE_HIGH   1
#define ULTRASONIC_TRIG_GPIO 15       // PIN_06 / P2.7
#define ULTRASONIC_TRIG_PIN  PIN_06
#define ULTRASONIC_ECHO_GPIO 16       // PIN_07 / P2.6  (5V! use a divider)
#define ULTRASONIC_ECHO_PIN  PIN_07

#endif // __HW_CONFIG_H__
