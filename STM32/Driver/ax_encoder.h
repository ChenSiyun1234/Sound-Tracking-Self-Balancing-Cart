/* ax_encoder.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_ENCODER_H
#define __AX_ENCODER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_ENCODER_A_Init(void);
uint16_t AX_ENCODER_A_GetCounter(void);
void AX_ENCODER_A_SetCounter(uint16_t count);

void AX_ENCODER_B_Init(void);
uint16_t AX_ENCODER_B_GetCounter(void);
void AX_ENCODER_B_SetCounter(uint16_t count);

#endif


