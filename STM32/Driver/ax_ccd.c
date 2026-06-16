/* ax_ccd.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_ccd.h"
#include "ax_sys.h"
#include <stdio.h>

#define TSL_CLK   PBout(13)   //CLK 
#define TSL_SI    PBout(12)   //SI  

static void CCD_Delay_us(void);
static uint16_t CCD_GetAdcData(void);


void AX_CCD_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);	

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);

	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);

	while(ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);

	while(ADC_GetCalibrationStatus(ADC1)); 		
}


void AX_CCD_GetData(uint16_t *pbuf)
{
	uint8_t i;
	
	TSL_CLK=1;
    TSL_SI=0; 
    CCD_Delay_us();
    
    TSL_SI=1; 
    TSL_CLK=0;
    CCD_Delay_us();
	
    TSL_CLK=1;
    TSL_SI=0;
    CCD_Delay_us(); 
	
	for(i=0;i<128;i++)					
	{   
		TSL_CLK=0; 
		CCD_Delay_us();
        
		*(pbuf+i) = (CCD_GetAdcData())>>4;
		TSL_CLK=1;
		CCD_Delay_us();	
	}
}


int16_t AX_CCD_GetOffset(void)
{
	uint8_t i;
	static uint16_t ccd[128]= {0};
	static uint16_t ccd_min,ccd_max;
	static uint16_t ccd_threshold;
	static uint16_t ccd_left,ccd_right;
	static int16_t ccd_offset;
	
	AX_CCD_GetData(ccd);
	
//	for(i=0; i<128; i++) printf("%d ",ccd[i]);
//	printf("\r\n");

	
	ccd_min = ccd[0];
	ccd_max = ccd[0];
	
	for(i=0; i<128; i++)
	{
		if(ccd_min > ccd[i])
		{
			ccd_min = ccd[i];
		}
		
		if(ccd_max < ccd[i])
		{
			ccd_max = ccd[i];
		}	
	}
	
	ccd_threshold =(ccd_min + (ccd_max-ccd_min)/2);	  
	
	
	for(i=0; i<125; i++)
	{
		//
		if(ccd[i] < ccd_threshold)
		{
			if((ccd[i+1]<ccd_threshold) && (ccd[i+2]<ccd_threshold) && (ccd[i+3]<ccd_threshold))
			{
				ccd_left = i;
				break;
			}
		}
	}
	
	for(i=128; i>2; i--)
	{
		//
		if(ccd[i] < ccd_threshold)
		{
			if((ccd[i-1]<ccd_threshold) && (ccd[i-2]<ccd_threshold) && (ccd[i-3]<ccd_threshold))
			{
				ccd_right = i;
				break;
			}
		}
	}
	
	ccd_offset = (ccd_left+ccd_right)/2 - 64;
	
	//printf("%d  %d \r\n",ccd_threshold, ccd_offset);
	
	return ccd_offset;
}


static uint16_t CCD_GetAdcData(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);  

	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
	
	return ADC_GetConversionValue(ADC1);
}


static void CCD_Delay_us(void)
{
   int i;   

   for(i=0;i<30;i++);      
}




