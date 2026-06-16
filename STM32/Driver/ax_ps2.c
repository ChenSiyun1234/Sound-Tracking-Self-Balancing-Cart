/* ax_ps2.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_ps2.h"
#include "ax_delay.h"
#include "ax_sys.h"

#define DI()     PCin(0)

#define CMD_H()  PCout(1)=1
#define CMD_L()  PCout(1)=0

#define CS_H()   PCout(2)=1
#define CS_L()   PCout(2)=0

#define CLK_H()  PCout(3)=1
#define CLK_L()  PCout(3)=0


const  uint8_t PS2_cmnd[9] = {0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t PS2_data[9] = {0};



void AX_PS2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC , ENABLE);
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
	CS_H();
}



static uint8_t PS2_ReadWriteData(uint8_t cmd)
{
	volatile uint8_t res = 0;
	volatile uint8_t ref;
	
	for(ref = 0x01; ref > 0x00; ref <<= 1)
	{
		if(ref&cmd)
			CMD_H();
		else
			CMD_L();
		
		CLK_L();
		AX_Delayus(16);
		
		if(DI())
			res |= ref; 
		CLK_H();
		AX_Delayus(16);		
	}

    return res;	
}


void AX_PS2_ScanKey(JOYSTICK_TypeDef *JoystickStruct)
{
	uint8_t i;
	
	CS_L();
	
	for(i=0; i<9; i++)
	{
		PS2_data[i] = PS2_ReadWriteData(PS2_cmnd[i]);
	}
	
	CS_H();

	JoystickStruct->mode = PS2_data[1];
	JoystickStruct->btn1 = ~PS2_data[3];
	JoystickStruct->btn2 = ~PS2_data[4];
	JoystickStruct->RJoy_LR = PS2_data[5];
	JoystickStruct->RJoy_UD = PS2_data[6];
	JoystickStruct->LJoy_LR = PS2_data[7];
	JoystickStruct->LJoy_UD = PS2_data[8];
}


