/* ax_sys.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_sys.h"


void AX_JTAG_Set(uint8_t mode)
{
	uint32_t temp;
	
	temp=mode;
	temp<<=25;
	RCC->APB2ENR|=1<<0;
	AFIO->MAPR&=0XF8FFFFFF;
	AFIO->MAPR|=temp;
} 


