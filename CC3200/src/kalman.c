//*****************************************************************************
// kalman.c  -  2-state IMU Kalman filter (see kalman.h)
//*****************************************************************************
#include "kalman.h"

void Kalman_Init(tKalman *pK, float Q_angle, float Q_bias, float R_measure)
{
    pK->Q_angle   = Q_angle;
    pK->Q_bias    = Q_bias;
    pK->R_measure = R_measure;
    pK->angle = 0.0f;
    pK->bias  = 0.0f;
    pK->rate  = 0.0f;
    pK->P[0][0] = 0.0f; pK->P[0][1] = 0.0f;
    pK->P[1][0] = 0.0f; pK->P[1][1] = 0.0f;
}

void Kalman_SetAngle(tKalman *pK, float fAngle)
{
    pK->angle = fAngle;
    pK->P[0][0] = 0.0f; pK->P[0][1] = 0.0f;
    pK->P[1][0] = 0.0f; pK->P[1][1] = 0.0f;
}

float Kalman_Update(tKalman *pK, float fNewAngle, float fNewRate, float fDt)
{
    float y, S, K0, K1, P00, P01;

    // ---- Predict ----
    pK->rate   = fNewRate - pK->bias;
    pK->angle += fDt * pK->rate;

    pK->P[0][0] += fDt * (fDt * pK->P[1][1] - pK->P[0][1] - pK->P[1][0] + pK->Q_angle);
    pK->P[0][1] -= fDt * pK->P[1][1];
    pK->P[1][0] -= fDt * pK->P[1][1];
    pK->P[1][1] += pK->Q_bias * fDt;

    // ---- Update ----
    y  = fNewAngle - pK->angle;            // innovation
    S  = pK->P[0][0] + pK->R_measure;      // innovation covariance
    K0 = pK->P[0][0] / S;                  // Kalman gain (angle)
    K1 = pK->P[1][0] / S;                  // Kalman gain (bias)

    pK->angle += K0 * y;
    pK->bias  += K1 * y;

    P00 = pK->P[0][0];
    P01 = pK->P[0][1];
    pK->P[0][0] -= K0 * P00;
    pK->P[0][1] -= K0 * P01;
    pK->P[1][0] -= K1 * P00;
    pK->P[1][1] -= K1 * P01;

    return pK->angle;
}
