/* ax_uart4.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_UART4_H
#define __AX_UART4_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

void    AX_UART4_Init(uint32_t baud);
uint8_t AX_UART4_GetData(uint8_t *pbuf);
void    AX_UART4_SendPacket(uint8_t *pbuf, uint8_t len, uint8_t num);

#endif 


