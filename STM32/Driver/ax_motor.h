/* ax_motor.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_MOTOR_H
#define __AX_MOTOR_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_MOTOR_Init(void);
void AX_MOTOR_A_SetSpeed(int16_t speed);
void AX_MOTOR_B_SetSpeed(int16_t speed);

#endif


