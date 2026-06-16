/* speed.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPEED_H
#define __SPEED_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

int16_t Motor_SpeedCtlA(float spd_target, float spd_current);
int16_t Motor_SpeedCtlB(float spd_target, float spd_current);
int16_t Motor_SpeedCtlC(float spd_target, float spd_current);
int16_t Motor_SpeedCtlD(float spd_target, float spd_current);


#endif


