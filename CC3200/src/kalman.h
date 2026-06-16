//*****************************************************************************
// kalman.h
//
// 2-state (angle + gyro-bias) Kalman filter fusing accel angle with gyro rate.
// After K. Lauszus / TKJ Electronics.
//*****************************************************************************
#ifndef __KALMAN_H__
#define __KALMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float Q_angle;     // process noise: accel angle
    float Q_bias;      // process noise: gyro bias
    float R_measure;   // measurement noise: accel angle
    float angle;       // fused angle estimate (deg)
    float bias;        // estimated gyro bias (deg/s)
    float rate;        // unbiased rate (deg/s)
    float P[2][2];     // error covariance
} tKalman;

extern void  Kalman_Init(tKalman *pK, float Q_angle, float Q_bias, float R_measure);

// Force the current angle and zero the covariance.
extern void  Kalman_SetAngle(tKalman *pK, float fAngle);

// fNewAngle = accel angle (deg), fNewRate = gyro rate (deg/s), fDt = seconds. Returns fused angle (deg).
extern float Kalman_Update(tKalman *pK, float fNewAngle, float fNewRate, float fDt);

#ifdef __cplusplus
}
#endif
#endif // __KALMAN_H__
