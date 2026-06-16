/* ax_beep.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_beep.h"


void AX_BEEP_Init(void) 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
	GPIO_ResetBits(GPIOC,GPIO_Pin_13);
	
}	



