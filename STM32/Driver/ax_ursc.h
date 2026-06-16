/* ax_ursc.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_URSC_H
#define __AX_URSC_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_URSC_Init(void);
void AX_URSC_StartUp(void);
uint32_t AX_URSC_GetDistance(void);


#endif


