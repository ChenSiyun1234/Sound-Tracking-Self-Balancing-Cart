/* ax_delay.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_DELAY_H
#define __AX_DELAY_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_DELAY_Init(void);
void AX_Delayus(uint32_t us);
void AX_Delayms(uint16_t ms);

#endif 


