/* ax_flash.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_FLASH_H
#define __AX_FLASH_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void AX_FLASH_Erase(void);
void AX_FLASH_Read(uint16_t addr, uint16_t *pbuff, uint8_t num);
void AX_FLASH_Write(uint16_t addr, uint16_t *pbuff, uint8_t num);


#endif 


