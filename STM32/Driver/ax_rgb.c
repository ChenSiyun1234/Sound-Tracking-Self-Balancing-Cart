/* ax_rgb.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_rgb.h"


//  __
// |  |_|   0b110  high level
//  _   
// | |__|   0b100  low level


#define TIM_ONE           0xF8
#define TIM_ZERO          0x80

uint8_t RGB_BYTE_Buffer[PIXEL_NUM*24+2] = {0};


void AX_RGB_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	DMA_InitTypeDef  DMA_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB, GPIO_Pin_15);	
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);   

	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	
	SPI_Init(SPI2, &SPI_InitStructure);

	SPI_Cmd(SPI2, ENABLE);
	SPI_CalculateCRC(SPI2, DISABLE);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel5);
	
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SPI2->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)RGB_BYTE_Buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);	
}



void RGB_Update(void)
{
	
	DMA_SetCurrDataCounter(DMA1_Channel5, PIXEL_NUM*24+2 );     // load number of bytes to be transferred
	
	DMA_Cmd(DMA1_Channel5, ENABLE);                             // enable DMA channel 3

	while (!DMA_GetFlagStatus(DMA1_FLAG_TC5));  
    
    DMA_Cmd(DMA1_Channel5, DISABLE); 
	
    DMA_ClearFlag(DMA1_FLAG_TC5);    
	
}




void AX_RGB_SetFullColor( uint8_t r, uint8_t g, uint8_t b)
{
	
	uint16_t i,j;

	RGB_BYTE_Buffer[0]=0;                               
	RGB_BYTE_Buffer[PIXEL_NUM*24 + 1]=0;	

	for(j=0;j<8;j++)
	{
		RGB_BYTE_Buffer[ j + 1] = ((g<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
		RGB_BYTE_Buffer[ j + 1 + 8] = ((r<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
		RGB_BYTE_Buffer[ j + 1 + 16] = ((b<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;			
	}

	for(i=1; i<PIXEL_NUM; i++ )
	{
		for(j=1;j<25;j++)
		{
			RGB_BYTE_Buffer[(24*i)+j] = RGB_BYTE_Buffer[j];  
		}		
	} 

	RGB_Update();
}



void AX_RGB_SetPixelColor(uint8_t pixel[PIXEL_NUM][3])
{
	
	uint8_t i,j;
	
    RGB_BYTE_Buffer[0]=0;                               
    RGB_BYTE_Buffer[PIXEL_NUM*24 + 1]=0;	
	
	for(i=0; i<PIXEL_NUM; i++ )
	{	
		for(j=0;j<8;j++)
		{
			RGB_BYTE_Buffer[(i*24) + j + 1]      = (( pixel[i][1]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
			RGB_BYTE_Buffer[(i*24) + j + 1 + 8]  = ((pixel[i][0]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
			RGB_BYTE_Buffer[(i*24) + j + 1 + 16] = (( pixel[i][2]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;	
		}	
	}
	
    RGB_Update();
}


void AX_RGB_SetPixelColor1(const uint8_t pixel[PIXEL_NUM][3])
{
	
	uint8_t i,j;
	
    RGB_BYTE_Buffer[0]=0;                               
    RGB_BYTE_Buffer[PIXEL_NUM*24 + 1]=0;	
	
	for(i=0; i<PIXEL_NUM; i++ )
	{	
		for(j=0;j<8;j++)
		{
			RGB_BYTE_Buffer[(i*24) + j + 1]      = (( pixel[i][1]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
			RGB_BYTE_Buffer[(i*24) + j + 1 + 8]  = ((pixel[i][0]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;
			RGB_BYTE_Buffer[(i*24) + j + 1 + 16] = (( pixel[i][2]<<j) & 0x0080) ? TIM_ONE:TIM_ZERO;	
		}	
	}

    RGB_Update();
	
}



