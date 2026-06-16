/* ax_speed.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_SPEED_H
#define __AX_SPEED_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

int16_t AX_SPEED_PidCtlA(float spd_target, float spd_current);
int16_t AX_SPEED_PidCtlB(float spd_target, float spd_current);


#endif


