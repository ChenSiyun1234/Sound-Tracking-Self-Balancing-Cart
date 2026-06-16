/* ax_vin.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_VIN_H
#define __AX_VIN_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_VIN_Init(void);
uint16_t AX_VIN_GetVol_X100(void);

#endif


