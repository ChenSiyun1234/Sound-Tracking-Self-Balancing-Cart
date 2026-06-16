/* ax_ps2.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_PS2_H
#define __AX_PS2_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

#define  PS2_BT1_SELECT     0x01
#define  PS2_BT1_JOY_L      0x02
#define  PS2_BT1_JOY_R      0x04
#define  PS2_BT1_START      0x08
#define  PS2_BT1_UP         0x10
#define  PS2_BT1_RIGHT      0x20
#define  PS2_BT1_DOWN       0x40
#define  PS2_BT1_LEFT       0x80

#define  PS2_BT2_L2         0x01
#define  PS2_BT2_R2         0x02
#define  PS2_BT2_L1         0x04
#define  PS2_BT2_R1         0x08
#define  PS2_BT2_Y          0x10
#define  PS2_BT2_B          0x20
#define  PS2_BT2_A          0x40
#define  PS2_BT2_X          0x80

typedef struct			 				
{
  uint8_t mode;

  uint8_t btn1;         /* B0:SLCT B1:JR  B0:JL B3:STRT B4:UP B5:R B6:DOWN  B7:L   */

  uint8_t btn2;         /* B0:L2   B1:R2  B2:L1 B3:R1   B4:Y  B5:B B6:A     B7:X */

  uint8_t RJoy_LR;

  uint8_t RJoy_UD;

  uint8_t LJoy_LR;

  uint8_t LJoy_UD;
	
}JOYSTICK_TypeDef;



void AX_PS2_Init(void);
void AX_PS2_ScanKey(JOYSTICK_TypeDef* JoystickStruct);

#endif 


