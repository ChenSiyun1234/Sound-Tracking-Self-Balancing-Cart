/* ax_laser.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_LASER_H
#define __AX_LASER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

#define LS_HEADER     0x54
#define LS_F_LEN      47
#define LS_VERLEN     0x2C
#define LS_PNAM       12

#define LS_DISABLE     0
#define LS_ENABLE      1

typedef struct
{
	uint16_t      angle;
	uint16_t   distance;

}LaserPointTypeDef;

extern LaserPointTypeDef ax_ls_point[250];


void AX_LASER_Init(void);
void AX_LASER_SetEnable(uint8_t state);

#endif 


