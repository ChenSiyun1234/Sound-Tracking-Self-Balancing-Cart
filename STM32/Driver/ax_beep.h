/* ax_beep.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_BEEP_H
#define __AX_BEEP_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_BEEP_Init(void);

#define AX_BEEP_On()  	     GPIO_SetBits(GPIOC, GPIO_Pin_13)
#define AX_BEEP_Off()		 GPIO_ResetBits(GPIOC, GPIO_Pin_13)
#define AX_BEEP_Toggle()     GPIO_WriteBit(GPIOC, GPIO_Pin_13, (BitAction) (1 - GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)))

#endif 


