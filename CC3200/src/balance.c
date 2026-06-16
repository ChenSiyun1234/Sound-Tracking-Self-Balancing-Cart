//*****************************************************************************
// balance.c  -  Kalman tilt fusion + cascaded control (see balance.h)
//
// Three loops summed every control tick:
//     angle PD   : keep the body upright   (tilt + gyro pitch rate)
//     velocity PI: stop drift              (wheel speed, encoders)
//     turn PD    : hold heading / steer    (yaw gyro rate)
//   left = base + turn   right = base - turn   (base = angle + velocity)
// Per-wheel direction is handled by MOTOR_L/R_INVERT in motor.c.
//*****************************************************************************
#include <math.h>
#include "balance.h"
#include "motor.h"          // MOTOR_LEVEL_MAX
#include "hw_config.h"
#include "bma222drv.h"
#include "mpu6050.h"
#include "kalman.h"
#include "encoder.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"

#define PI_F        3.14159265f
#define RAD2DEG(x)  ((x) * (180.0f / PI_F))

//*****************************************************************************
// Measurement state
//*****************************************************************************
static tKalman g_sKalman;
static float g_fTilt        = 0.0f;   // fused tilt (deg)
static float g_fAccelOffset = 0.0f;   // accel "upright" reference (deg)
static float g_fGyroBias    = 0.0f;   // static gyro zero-rate (deg/s)
static float g_fPitchRate   = 0.0f;   // latest pitch gyro rate (deg/s) -> D term
static float g_fYawRate     = 0.0f;   // latest yaw  gyro rate (deg/s) -> turn loop
static float g_fWheelVel    = 0.0f;   // averaged wheel speed (m/s, +forward)

//*****************************************************************************
// Controller gains / state
//*****************************************************************************
// Angle loop (runtime-adjustable over serial)
static float g_fKp = BALANCE_KP, g_fKi = BALANCE_KI, g_fKd = BALANCE_KD;
static float g_fIntegral = 0.0f;

// Velocity loop (runtime-adjustable)
static float g_fVKp = VELOCITY_KP, g_fVKi = VELOCITY_KI;
static float g_fVelBias = 0.0f, g_fVelInt = 0.0f;

// Turn loop (incremental)
static float g_fTurnBiasLast = 0.0f, g_fTurnOut = 0.0f;

//*****************************************************************************
// Read the accel angle (from the configured source) + the MPU pitch & yaw rates.
//*****************************************************************************
static int ReadSensors(float *pfAccelAngle, float *pfGyroRate)
{
    float fGyro[3];
    float fFwd, fVert;

#if ACCEL_SOURCE_IS_BMA222
    // Gyro from MPU, accel angle from on-board BMA222.
    if (MPU6050_Read(0, fGyro) != 0)
    {
#if ALLOW_NO_GYRO
        fGyro[0] = fGyro[1] = fGyro[2] = 0.0f;   // accel-only fallback
#else
        return -1;
#endif
    }
    {
        signed char a[3];
        if (BMA222Read(&a[0], &a[1], &a[2]) != 0) return -1;
        fFwd  = (float)a[BMA_FORWARD_AXIS]  * (float)BMA_FORWARD_SIGN;
        fVert = (float)a[BMA_VERTICAL_AXIS] * (float)BMA_VERTICAL_SIGN;
    }
#else
    // Accel and gyro from the MPU in one block read.
    float fAcc[3];
    if (MPU6050_Read(fAcc, fGyro) != 0)
    {
#if ALLOW_NO_GYRO
        fGyro[0] = fGyro[1] = fGyro[2] = 0.0f;
        fAcc[0] = 0.0f; fAcc[1] = 0.0f; fAcc[2] = 1.0f;
#else
        return -1;
#endif
    }
    fFwd  = fAcc[MPU_FORWARD_AXIS]  * (float)MPU_FORWARD_SIGN;
    fVert = fAcc[MPU_VERTICAL_AXIS] * (float)MPU_VERTICAL_SIGN;
#endif

    *pfGyroRate = fGyro[MPU_GYRO_PITCH_AXIS] * (float)MPU_GYRO_PITCH_SIGN - g_fGyroBias;
    g_fYawRate  = fGyro[MPU_GYRO_YAW_AXIS]   * (float)MPU_GYRO_YAW_SIGN;

    *pfAccelAngle = RAD2DEG(atan2f(fFwd, fVert)) - g_fAccelOffset;
    return 0;
}

//*****************************************************************************
void Balance_Init(void)
{
    Kalman_Init(&g_sKalman, KALMAN_Q_ANGLE, KALMAN_Q_BIAS, KALMAN_R_MEASURE);
    g_fTilt = g_fAccelOffset = g_fGyroBias = 0.0f;
    g_fPitchRate = g_fYawRate = g_fWheelVel = 0.0f;
    g_fIntegral  = 0.0f;
    g_fVelBias = g_fVelInt = 0.0f;
    g_fTurnBiasLast = g_fTurnOut = 0.0f;
}

//*****************************************************************************
int Balance_Calibrate(void)
{
    int i;
    const int N = 64;
    float fSum = 0.0f, fAngle, fRate;

    // 1) Static gyro bias (cart must be still).
    g_fGyroBias = 0.0f;
    g_fGyroBias = MPU6050_CalibrateGyroBias(MPU_GYRO_PITCH_AXIS) * (float)MPU_GYRO_PITCH_SIGN;

    // 2) Accelerometer upright reference. Skip occasional I2C glitches.
    g_fAccelOffset = 0.0f;
    {
        int iTries = 0;
        for (i = 0; i < N && iTries < N * 16; iTries++)
        {
            if (ReadSensors(&fAngle, &fRate) == 0) { fSum += fAngle; i++; }
            MAP_UtilsDelay(80000000 / 3 / 200);    // ~5 ms
        }
        if (i < N) return -1;                      // sensor truly unreadable
    }
    g_fAccelOffset = fSum / (float)N;

    // 3) Seed the filter at upright and clear all loop state.
    Kalman_SetAngle(&g_sKalman, 0.0f);
    g_fTilt = 0.0f;
    g_fIntegral = 0.0f;
    g_fVelBias = g_fVelInt = 0.0f;
    g_fTurnBiasLast = g_fTurnOut = 0.0f;
    Encoder_Reset();
    return 0;
}

//*****************************************************************************
int Balance_Sample(float fDt)
{
    float fAccelAngle, fGyroRate;
    long  lL, lR;

    if (ReadSensors(&fAccelAngle, &fGyroRate) != 0)
        return -1;
    if (fDt <= 0.0f) fDt = BALANCE_LOOP_DT_S;

    g_fTilt      = Kalman_Update(&g_sKalman, fAccelAngle, fGyroRate, fDt);
    g_fPitchRate = fGyroRate;                       // gyro rate -> angle D term

    // Wheel speed: counts accumulated this period -> m/s, then zero the counters.
    lL = Encoder_GetLeftCount();
    lR = Encoder_GetRightCount();
    Encoder_Reset();
    g_fWheelVel = ((float)(lL + lR) * 0.5f) * WHEEL_M_PER_COUNT / fDt;
    return 0;
}

//*****************************************************************************
float Balance_GetTilt(void)     { return g_fTilt; }
float Balance_GetRate(void)     { return g_sKalman.rate; }
float Balance_GetWheelVel(void) { return g_fWheelVel; }

int Balance_IsFallen(void)
{
    float a = g_fTilt < 0 ? -g_fTilt : g_fTilt;
    return (a > BALANCE_SAFE_TILT_DEG);
}

//*****************************************************************************
void Balance_ResetPID(void)
{
    g_fIntegral = 0.0f;
    g_fVelBias = g_fVelInt = 0.0f;
    g_fTurnBiasLast = g_fTurnOut = 0.0f;
}

//*****************************************************************************
// Cascaded controller -> per-wheel motor commands (each in [-MAX..MAX]).
//   iTargetVxMmps : desired forward speed (mm/s, +forward; 0 to stand still)
//   iTargetWDps   : desired turn rate     (deg/s; 0 to hold heading)
//*****************************************************************************
void Balance_ComputeMotors(float fDt, int iTargetVxMmps, int iTargetWDps,
                           int *piLeft, int *piRight)
{
    float fErr, fBase, fTurn = 0.0f, fL, fR;

    if (fDt <= 0.0f) fDt = BALANCE_LOOP_DT_S;

    //--- Angle PD (target tilt = 0): out = Kp*tilt + Ki*Int + Kd*gyroRate -------
    fErr = g_fTilt;
    g_fIntegral += fErr * fDt;
    if (g_fIntegral >  BALANCE_I_CLAMP) g_fIntegral =  BALANCE_I_CLAMP;
    if (g_fIntegral < -BALANCE_I_CLAMP) g_fIntegral = -BALANCE_I_CLAMP;
    fBase = g_fKp * fErr + g_fKi * g_fIntegral + g_fKd * g_fPitchRate;

    //--- Velocity PI (opposes drift): bias = lowpass(0 - speed) -----------------
#if USE_VELOCITY_LOOP
    {
        float fBiasNew = 0.0f - g_fWheelVel;
        g_fVelBias = g_fVelBias * (1.0f - VELOCITY_LPF_ALPHA)
                   + fBiasNew  *  VELOCITY_LPF_ALPHA;
        g_fVelInt += g_fVelBias * fDt;                          // measured displacement
        g_fVelInt += (float)iTargetVxMmps * 0.001f * fDt;       // commanded displacement
        if (g_fVelInt >  VELOCITY_INT_CLAMP) g_fVelInt =  VELOCITY_INT_CLAMP;
        if (g_fVelInt < -VELOCITY_INT_CLAMP) g_fVelInt = -VELOCITY_INT_CLAMP;
        fBase += g_fVKp * g_fVelBias + g_fVKi * g_fVelInt;
    }
#else
    (void)iTargetVxMmps;
#endif

    //--- Turn PD (incremental; holds heading at target 0) ----------------------
#if USE_TURN_LOOP
    {
        float fBias = (float)iTargetWDps - g_fYawRate;
        g_fTurnOut += TURN_KP * fBias + TURN_KD * (fBias - g_fTurnBiasLast);
        g_fTurnBiasLast = fBias;
        if (g_fTurnOut >  (float)MOTOR_LEVEL_MAX) g_fTurnOut =  (float)MOTOR_LEVEL_MAX;
        if (g_fTurnOut < -(float)MOTOR_LEVEL_MAX) g_fTurnOut = -(float)MOTOR_LEVEL_MAX;
        fTurn = g_fTurnOut;
    }
#else
    (void)iTargetWDps;
#endif

    //--- Mix + clamp -----------------------------------------------------------
    fL = fBase + fTurn;
    fR = fBase - fTurn;
    if (fL >  (float)MOTOR_LEVEL_MAX) fL =  (float)MOTOR_LEVEL_MAX;
    if (fL < -(float)MOTOR_LEVEL_MAX) fL = -(float)MOTOR_LEVEL_MAX;
    if (fR >  (float)MOTOR_LEVEL_MAX) fR =  (float)MOTOR_LEVEL_MAX;
    if (fR < -(float)MOTOR_LEVEL_MAX) fR = -(float)MOTOR_LEVEL_MAX;
    *piLeft  = (int)fL;
    *piRight = (int)fR;
}

//*****************************************************************************
// Angle-only output (single value for both wheels).
//*****************************************************************************
int Balance_Update(float fTargetTilt, float fDt)
{
    int iL, iR;
    Balance_ComputeMotors(fDt, 0, 0, &iL, &iR);
    (void)fTargetTilt;
    return iL;
}

//*****************************************************************************
// Live gain access (for serial tuning).
//*****************************************************************************
void Balance_SetGains(float fKp, float fKi, float fKd)
{
    g_fKp = fKp; g_fKi = fKi; g_fKd = fKd;
    g_fIntegral = 0.0f;
}
void Balance_GetGains(float *pfKp, float *pfKi, float *pfKd)
{
    if (pfKp) *pfKp = g_fKp;
    if (pfKi) *pfKi = g_fKi;
    if (pfKd) *pfKd = g_fKd;
}
void Balance_SetVelGains(float fVKp, float fVKi)
{
    g_fVKp = fVKp; g_fVKi = fVKi;
    g_fVelInt = 0.0f;
}
void Balance_GetVelGains(float *pfVKp, float *pfVKi)
{
    if (pfVKp) *pfVKp = g_fVKp;
    if (pfVKi) *pfVKi = g_fVKi;
}
