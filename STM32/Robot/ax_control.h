/* ax_control.h - STM32 self-balancing cart firmware (discrete C8T6 build). */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_CONTROL_H
#define __AX_CONTROL_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_CTL_Cmd(void);     /* apply pending HC-05 character commands */

#endif
