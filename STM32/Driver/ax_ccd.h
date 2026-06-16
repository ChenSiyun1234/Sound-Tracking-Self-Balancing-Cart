/* ax_ccd.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_CCD_H
#define __AX_CCD_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_CCD_Init(void);
void AX_CCD_GetData(uint16_t *pbuf);
int16_t AX_CCD_GetOffset(void);


#endif


