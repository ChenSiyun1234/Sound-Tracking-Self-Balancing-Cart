/* ax_key.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_key.h"
#include "ax_delay.h"
#include "ax_sys.h"


void AX_KEY_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


uint8_t AX_KEY_Scan(void)
{
	if(PBin(0) == 0)		
	{
		return 1;
	}
	
	return 0;
}




