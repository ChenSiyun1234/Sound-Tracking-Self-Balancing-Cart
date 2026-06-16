//*****************************************************************************
// mpu6050.c  -  MPU-6050 6-axis IMU driver (see mpu6050.h)
//*****************************************************************************
#include "mpu6050.h"
#include "hw_config.h"
#include "i2c_if.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"

//*****************************************************************************
// Register map (subset)
//*****************************************************************************
#define MPU_REG_SMPLRT_DIV      0x19
#define MPU_REG_CONFIG          0x1A   // DLPF
#define MPU_REG_GYRO_CONFIG     0x1B
#define MPU_REG_ACCEL_CONFIG    0x1C
#define MPU_REG_ACCEL_XOUT_H    0x3B   // 14 bytes: AX,AY,AZ,TEMP,GX,GY,GZ
#define MPU_REG_PWR_MGMT_1      0x6B
#define MPU_REG_WHO_AM_I        0x75

// Full-scale sensitivities for the ranges selected below.
#define ACC_LSB_PER_G           16384.0f   // +/-2 g
#define GYRO_LSB_PER_DPS        131.0f     // +/-250 dps

//*****************************************************************************
static int WriteReg(unsigned char ucReg, unsigned char ucVal)
{
    unsigned char buf[2] = { ucReg, ucVal };
    return I2C_IF_Write(MPU6050_I2C_ADDR, buf, 2, 1);
}

static int ReadBlock(unsigned char ucReg, unsigned char *pucBuf, unsigned char ucLen)
{
    return I2C_IF_ReadFrom(MPU6050_I2C_ADDR, &ucReg, 1, pucBuf, ucLen);
}

// Write a config register, then read back and retry until it matches.
static int WriteRegVerify(unsigned char ucReg, unsigned char ucVal)
{
    int i;
    unsigned char ucRb;
    for (i = 0; i < 6; i++)
    {
        if (WriteReg(ucReg, ucVal) == 0 &&
            ReadBlock(ucReg, &ucRb, 1) == 0 && ucRb == ucVal)
            return 0;
        MAP_UtilsDelay(80000000 / 3 / 500);        // ~2 ms, then retry
    }
    return -1;
}

//*****************************************************************************
int MPU6050_Init(void)
{
    unsigned char ucWho = 0;
    int iAttempt;

    for (iAttempt = 0; iAttempt < 4; iAttempt++)
    {
        // Wake from sleep, use X-gyro PLL clock.
        if (WriteRegVerify(MPU_REG_PWR_MGMT_1, 0x01) != 0) continue;
        MAP_UtilsDelay(80000000 / 3 / 100);        // ~10 ms to settle

        if (ReadBlock(MPU_REG_WHO_AM_I, &ucWho, 1) < 0) continue;
        // Accept any non-zero ID; reject 0x00/0xFF (no device / dead bus).
        if (ucWho == 0x00 || ucWho == 0xFF) continue;

        if (WriteRegVerify(MPU_REG_SMPLRT_DIV,   0x00) != 0) continue;  // 1 kHz
        if (WriteRegVerify(MPU_REG_CONFIG,       0x03) != 0) continue;  // DLPF ~44 Hz
        if (WriteRegVerify(MPU_REG_GYRO_CONFIG,  0x00) != 0) continue;  // +/-250 dps
        if (WriteRegVerify(MPU_REG_ACCEL_CONFIG, 0x00) != 0) continue;  // +/-2 g
        return 0;                                  // all registers verified
    }
    return -1;
}

//*****************************************************************************
int MPU6050_WhoAmI(void)
{
    unsigned char ucWho = 0;
    if (ReadBlock(MPU_REG_WHO_AM_I, &ucWho, 1) < 0)
        return -1;
    return (int)ucWho;
}

//*****************************************************************************
int MPU6050_Read(float *pfAccG, float *pfGyroDps)
{
    unsigned char b[14];
    short s[7];
    int i;

    if (ReadBlock(MPU_REG_ACCEL_XOUT_H, b, 14) < 0)
        return -1;

    // Combine big-endian byte pairs into signed 16-bit values.
    for (i = 0; i < 7; i++)
        s[i] = (short)((b[i * 2] << 8) | b[i * 2 + 1]);

    if (pfAccG)
    {
        pfAccG[0] = (float)s[0] / ACC_LSB_PER_G;
        pfAccG[1] = (float)s[1] / ACC_LSB_PER_G;
        pfAccG[2] = (float)s[2] / ACC_LSB_PER_G;
    }
    if (pfGyroDps)                                  // s[3] is temperature, skip
    {
        pfGyroDps[0] = (float)s[4] / GYRO_LSB_PER_DPS;
        pfGyroDps[1] = (float)s[5] / GYRO_LSB_PER_DPS;
        pfGyroDps[2] = (float)s[6] / GYRO_LSB_PER_DPS;
    }
    return 0;
}

//*****************************************************************************
float MPU6050_CalibrateGyroBias(int iAxis)
{
    int i;
    const int N = 200;
    float fSum = 0.0f, fGyro[3];

    if (iAxis < 0 || iAxis > 2) iAxis = 0;
    for (i = 0; i < N; i++)
    {
        if (MPU6050_Read(0, fGyro) == 0)
            fSum += fGyro[iAxis];
        MAP_UtilsDelay(80000000 / 3 / 1000);       // ~1 ms between samples
    }
    return fSum / (float)N;
}
