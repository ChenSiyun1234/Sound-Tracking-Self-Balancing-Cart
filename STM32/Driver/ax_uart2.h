/* ax_uart2.h - HC-05 Bluetooth on USART2 (PA2=TX, PA3=RX), simple command link.
 *
 * The phone sends single-character commands (from a generic Bluetooth serial app
 * such as "Serial Bluetooth Terminal"); the RX interrupt buffers them and
 * AX_UART2_GetCmd() pops one at a time. See ax_control.c for the command set.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_UART2_H
#define __AX_UART2_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void    AX_UART2_Init(uint32_t baud);
uint8_t AX_UART2_GetCmd(void);   /* next received byte, or 0 if none pending */

#endif
