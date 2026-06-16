/* ax_sbus.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_SBUS_H
#define __AX_SBUS_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_SBUS_Init(void);
uint8_t AX_SBUS_GetRxData(uint16_t *pbuf);


#endif 


