/* ax_servo.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_servo.h" 


void AX_SERVO_S1234_Init(void)
{ 
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);	

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

	TIM_TimeBaseStructure.TIM_Prescaler=72-1;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=20000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

	//PWM1 Mode configuration
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM8, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);

	//PWM1 Mode configuration
	TIM_OC2Init(TIM8, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM8, TIM_OCPreload_Enable);
	
	//PWM1 Mode configuration
	TIM_OC3Init(TIM8, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);
	
	//PWM1 Mode configuration
	TIM_OC4Init(TIM8, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM8, ENABLE);

	TIM_Cmd(TIM8, ENABLE);	
	
	TIM_CtrlPWMOutputs(TIM8,ENABLE);		
	
}


void AX_SERVO_S1_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare4(TIM8,(1.111f*angle + 1500));
}


void AX_SERVO_S2_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare3(TIM8,(1.111f*angle + 1500));
	
}


void AX_SERVO_S3_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare2(TIM8,(1.111f*angle + 1500));
	
}


void AX_SERVO_S4_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare1(TIM8,(1.111f*angle + 1500));
	
}


void AX_SERVO_S56_Init(void)
{ 
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure; 	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);


	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseStructure.TIM_Prescaler=72-1;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=20000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	//PWM1 Mode configuration: Channel
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM5, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM5, TIM_OCPreload_Enable);

	//PWM1 Mode configuration: Channel
	TIM_OC2Init(TIM5, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM5, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM5, ENABLE);

	TIM_Cmd(TIM5, ENABLE);		
}


void AX_SERVO_S5_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare1(TIM5,(1.111f*angle + 1500));
	
}


void AX_SERVO_S6_SetAngle(int16_t angle)
{
	if(angle >  900) angle =  900;
	if(angle < -900) angle = -900;
	
	TIM_SetCompare2(TIM5,(1.111f*angle + 1500));
	
}


