/* ax_key.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_KEY_H
#define __AX_KEY_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_KEY_Init(void);
uint8_t AX_KEY_Scan(void);

#endif 


