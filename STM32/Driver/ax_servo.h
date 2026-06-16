/* ax_servo.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_SERVO_H
#define __AX_SERVO_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_SERVO_S1234_Init(void);
void AX_SERVO_S1_SetAngle(int16_t angle);
void AX_SERVO_S2_SetAngle(int16_t angle);
void AX_SERVO_S3_SetAngle(int16_t angle);
void AX_SERVO_S4_SetAngle(int16_t angle);

void AX_SERVO_S56_Init(void);
void AX_SERVO_S5_SetAngle(int16_t angle);
void AX_SERVO_S6_SetAngle(int16_t angle);

#endif


