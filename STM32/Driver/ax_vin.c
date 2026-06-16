/* ax_vin.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_vin.h"

#define ADC_REVISE  99.9


void AX_VIN_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);	

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_9, 1, ADC_SampleTime_239Cycles5);

	ADC_Cmd(ADC2, ENABLE);

	ADC_ResetCalibration(ADC2);

	while(ADC_GetResetCalibrationStatus(ADC2));

	ADC_StartCalibration(ADC2);

	while(ADC_GetCalibrationStatus(ADC2)); 	
}



uint16_t AX_VIN_GetVol_X100(void)
{
	uint16_t Input_Vol,temp;

	ADC_SoftwareStartConvCmd(ADC2, ENABLE);  

	while(!ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC ));

	temp = ADC_GetConversionValue(ADC2);
	
	Input_Vol = (uint16_t)((3.3*11.0*ADC_REVISE * temp)/4095);	
	
	return Input_Vol;
}



