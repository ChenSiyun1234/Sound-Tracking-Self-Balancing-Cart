//*****************************************************************************
// motor.h
//
// TB6612FNG dual DC-motor driver (TK-MD220A-V1.0). Per motor: one PWM input
// (PWMA/PWMB) for speed, two direction pins (AIN1/AIN2, BIN1/BIN2), plus a
// shared STBY pin that must be HIGH to drive. Channel A = left wheel,
// channel B = right wheel. Pin choices live in hw_config.h.
//*****************************************************************************
#ifndef __MOTOR_H__
#define __MOTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

// Speed command range: -MOTOR_LEVEL_MAX..+MOTOR_LEVEL_MAX (+ = forward, 0 = coast).
#define MOTOR_LEVEL_MAX     255

// Configure PWM timers, direction GPIOs and STBY. Call after PinMuxConfig();
// driver starts in STANDBY until Motor_Enable(1).
extern void Motor_Init(void);

// Enable (1) or disable (0) the H-bridge via STBY.
extern void Motor_Enable(unsigned char ucEnable);

// Signed wheel speed, clamped to +/-MOTOR_LEVEL_MAX (+ = forward). Per-wheel
// inversion handled internally from hw_config.h flags.
extern void Motor_SetLeft(int iSpeed);
extern void Motor_SetRight(int iSpeed);

extern void Motor_SetBoth(int iLeft, int iRight);

// Brake = both direction pins HIGH; coast = both LOW.
extern void Motor_Brake(void);
extern void Motor_Coast(void);

#ifdef __cplusplus
}
#endif
#endif // __MOTOR_H__
