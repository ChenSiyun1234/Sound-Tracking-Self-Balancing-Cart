/* ax_function.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_FUNCTION_H
#define __AX_FUNCTION_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"


extern int16_t ax_ccd_speed; 
extern int16_t ax_ccd_offset;   
extern int16_t ax_ccd_kp;
extern int16_t ax_ccd_kd;  


void AX_FUN_List1(void);
void AX_FUN_List2(void);
void AX_FUN_List3(void);
void AX_FUN_List4(void);
void AX_FUN_List5(void);

#endif


