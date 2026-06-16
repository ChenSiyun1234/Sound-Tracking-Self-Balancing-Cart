/* ax_rgb.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_RGB_H
#define __AX_RGB_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

#define  PIXEL_NUM  8

void AX_RGB_Init(void);
void AX_RGB_SetFullColor(uint8_t b, uint8_t r, uint8_t g);
void AX_RGB_SetPixelColor(uint8_t pixel[PIXEL_NUM][3]);
void AX_RGB_SetPixelColor1( const uint8_t pixel[PIXEL_NUM][3]);


#endif


