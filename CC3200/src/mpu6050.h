//*****************************************************************************
// mpu6050.h
//
// InvenSense MPU-6050 6-axis IMU driver (GY-521 module) on the shared I2C0
// bus (PIN_01/PIN_02). Provides the gyroscope (angular rate) fused with the
// BMA222 accelerometer angle in kalman.c for the tilt estimate.
//
// I2C address 0x68 (AD0 = GND). BMA222 is 0x18, OLED is 0x3C.
//*****************************************************************************
#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif

//! Wake the device, set ranges (+/-2 g, +/-250 dps) and the digital low-pass
//! filter, and verify WHO_AM_I. Call after I2C_IF_Open().
//! \return 0 on success, <0 if the device did not respond / WHO_AM_I mismatch.
extern int MPU6050_Init(void);

//! Read all six axes in one I2C burst.
//!   pfAccG[0..2]    = accelerometer X/Y/Z in g
//!   pfGyroDps[0..2] = gyroscope X/Y/Z in deg/s
//! Either pointer may be NULL.
//! \return 0 on success, <0 on I2C error.
extern int MPU6050_Read(float *pfAccG, float *pfGyroDps);

//! Average the gyro at rest to estimate its zero-rate bias (deg/s) on the
//! pitch axis. Keep the cart still while this runs (~0.3 s).
extern float MPU6050_CalibrateGyroBias(int iAxis);

//! Read the WHO_AM_I register (0x75). Returns the ID byte (0x68 on genuine
//! parts; clones may differ), or -1 on I2C error.
extern int MPU6050_WhoAmI(void);

#ifdef __cplusplus
}
#endif
#endif // __MPU6050_H__
