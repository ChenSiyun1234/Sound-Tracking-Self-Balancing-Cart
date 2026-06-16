//*****************************************************************************
// balance.h
//
// Tilt estimation + PID balance controller for the two-wheel cart.
//
// Accelerometer angle source selectable in hw_config.h (BMA222 default, or
// MPU-6050). Gyro rate always from MPU-6050. 2-state Kalman (kalman.c) fuses
// them into the tilt angle.
//
// Per tick:  Balance_Sample(dt)  ->  Balance_Update(target,dt)
//*****************************************************************************
#ifndef __BALANCE_H__
#define __BALANCE_H__

#ifdef __cplusplus
extern "C" {
#endif

//! Init Kalman + PID state. Call after I2C_IF_Open(), BMA222Open() and
//! MPU6050_Init().
extern void Balance_Init(void);

//! Cart must be held upright and still: measures gyro zero bias and the
//! accelerometer "0 degree" reference, seeds the filter. Blocks ~0.5 s.
//! \return 0 on success, <0 on sensor failure.
extern int Balance_Calibrate(void);

//! Read the IMU once and run one Kalman step. fDt = seconds since last call.
//! Call once per control tick BEFORE Balance_Update(). \return 0 / <0 on error.
extern int Balance_Sample(float fDt);

//! Latest fused tilt in degrees (0 = upright, + = leaning forward).
extern float Balance_GetTilt(void);

//! Latest unbiased pitch rate (deg/s) from the filter (useful for telemetry).
extern float Balance_GetRate(void);

//! True if |tilt| exceeds the safe limit (the cart is falling / has fallen).
extern int Balance_IsFallen(void);

//! Cascaded controller -> per-wheel motor commands (angle PD + velocity PI +
//! turn PD). Call once per tick after Balance_Sample().
//!   iTargetVxMmps = forward speed setpoint (mm/s, 0 = stand still)
//!   iTargetWDps   = turn-rate setpoint (deg/s, 0 = hold heading)
//! Pass *piLeft to Motor_SetLeft() and *piRight to Motor_SetRight().
extern void Balance_ComputeMotors(float fDt, int iTargetVxMmps, int iTargetWDps,
                                  int *piLeft, int *piRight);

//! Angle-only PID step (single value for both wheels). Convenience wrapper.
extern int Balance_Update(float fTargetTilt, float fDt);

//! Latest averaged wheel speed (m/s, +forward) from the encoders (telemetry).
extern float Balance_GetWheelVel(void);

//! Clear all loop integrators/history (on entering balance / recovery).
extern void Balance_ResetPID(void);

//! Live gain tuning.
extern void Balance_SetGains(float fKp, float fKi, float fKd);     // angle Kp/Ki/Kd
extern void Balance_GetGains(float *pfKp, float *pfKi, float *pfKd);
extern void Balance_SetVelGains(float fVKp, float fVKi);           // velocity Kp/Ki
extern void Balance_GetVelGains(float *pfVKp, float *pfVKi);

#ifdef __cplusplus
}
#endif
#endif // __BALANCE_H__
