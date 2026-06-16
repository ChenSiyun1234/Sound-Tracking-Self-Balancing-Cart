/* ax_delay.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_delay.h"

#include "FreeRTOS.h"

static u8  fac_us=0;


void AX_DELAY_Init(void) 
{	
	u32 reload;
	
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	
	fac_us=SystemCoreClock/1000000;
	reload=SystemCoreClock/1000000;
	
	reload*=1000000/configTICK_RATE_HZ;
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD=reload;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;
}


void AX_Delayus(uint32_t us)
{
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;
	
	ticks=us*fac_us;
	told=SysTick->VAL;
	
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)
				tcnt+=told-tnow;
			else 
				tcnt+=reload-tnow+told;	  
			
			told=tnow;
			
			if(tcnt>=ticks)
				break;
		}  
	}										       				
}


void AX_Delayms(uint16_t ms)
{
	AX_Delayus((uint32_t)(ms*1000));
	
}



