/* ax_uart1.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_UART1_H
#define __AX_UART1_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_UART1_Init(uint32_t baud);
void AX_UART1_SendPacket(uint8_t *pbuf, uint8_t len, uint8_t num);


#endif 


