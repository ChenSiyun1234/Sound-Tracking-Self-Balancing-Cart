/* ax_mpu6050_dmp.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */
 
  
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_MPU6050_DMP_H
#define __AX_MPU6050_DMP_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"


void AX_MPU6050_DMP_Init(void);
void AX_MPU6050_DMP_GetData(int16_t *gyro, int16_t *acc, int16_t *angle);
//void AX_MPU6050_DMP_GetData(int16_t *data);  

#endif


