/* ax_sbus.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_sbus.h"
#include <stdio.h>
#include "ax_robot.h"

#define SBUS_RECV_MAX    25
#define SBUS_START       0x0F
#define SBUS_END         0x00


static uint8_t  uart_sbus_rx_ok = 0;
static uint8_t  uart_sbus_rx_con=0;
static uint8_t  uart_sbus_rx_buf[40];

static uint16_t sbus_buf[2];


void AX_SBUS_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_InitStructure.USART_BaudRate = 100000;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_2;
	USART_InitStructure.USART_Parity = USART_Parity_Even;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx;
	USART_Init(USART3, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);

	USART_Cmd(USART3, ENABLE);
}


void USART3_IRQHandler(void)                	
{
	uint8_t Res;
	
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  
	{
		Res =USART_ReceiveData(USART3);	
		
		if(uart_sbus_rx_con == 0)
		{
			if(Res == SBUS_START)
			{
				uart_sbus_rx_buf[uart_sbus_rx_con] = Res;
				
				uart_sbus_rx_con++; 
			}			
		}
		else
		{
			if((uart_sbus_rx_con >= (SBUS_RECV_MAX-1)) && (Res ==  SBUS_END))
			{
				uart_sbus_rx_ok = 1;
				
				if(ax_control_mode != CTL_RMS)
				{
					sbus_buf[0] = ((int16_t)uart_sbus_rx_buf[ 3] >> 6 | ((int16_t)uart_sbus_rx_buf[ 4] << 2 )  | (int16_t)uart_sbus_rx_buf[ 5] << 10 ) & 0x07FF;
					sbus_buf[1] = ((int16_t)uart_sbus_rx_buf[10] >> 5 | ((int16_t)uart_sbus_rx_buf[11] << 3 )) & 0x07FF;
					
					if((sbus_buf[0] > 1500) && (sbus_buf[1] > 1500))
					{
						ax_control_mode = CTL_RMS;
						
						ax_beep_ring = BEEP_SHORT;
					}					
				}
				
				uart_sbus_rx_con = 0;
			}
			else
			{
				uart_sbus_rx_buf[uart_sbus_rx_con] = Res;
				
				uart_sbus_rx_con++; 
			}
		}			
	}
}


uint8_t AX_SBUS_GetRxData(uint16_t *pbuf)
{

	if(uart_sbus_rx_ok != 0)
	{
		*(pbuf+0) = ((int16_t)uart_sbus_rx_buf[ 1] >> 0 | ((int16_t)uart_sbus_rx_buf[ 2] << 8 )) & 0x07FF;
		*(pbuf+1) = ((int16_t)uart_sbus_rx_buf[ 2] >> 3 | ((int16_t)uart_sbus_rx_buf[ 3] << 5 )) & 0x07FF;
		*(pbuf+2) = ((int16_t)uart_sbus_rx_buf[ 3] >> 6 | ((int16_t)uart_sbus_rx_buf[ 4] << 2 )  | (int16_t)uart_sbus_rx_buf[ 5] << 10 ) & 0x07FF;
		*(pbuf+3) = ((int16_t)uart_sbus_rx_buf[ 5] >> 1 | ((int16_t)uart_sbus_rx_buf[ 6] << 7 )) & 0x07FF;
		*(pbuf+4) = ((int16_t)uart_sbus_rx_buf[ 6] >> 4 | ((int16_t)uart_sbus_rx_buf[ 7] << 4 )) & 0x07FF;
		*(pbuf+5) = ((int16_t)uart_sbus_rx_buf[ 7] >> 7 | ((int16_t)uart_sbus_rx_buf[ 8] << 1 )  | (int16_t)uart_sbus_rx_buf[9] <<  9 ) & 0x07FF;
		*(pbuf+6) = ((int16_t)uart_sbus_rx_buf[9] >> 2 | ((int16_t)uart_sbus_rx_buf[10] << 6 )) & 0x07FF;
		*(pbuf+7) = ((int16_t)uart_sbus_rx_buf[10] >> 5 | ((int16_t)uart_sbus_rx_buf[11] << 3 )) & 0x07FF;
		
//		*(pbuf+8) = ((int16_t)uart_sbus_rx_buf[12] << 0 | ((int16_t)uart_sbus_rx_buf[13] << 8 )) & 0x07FF;
//		*(pbuf+9) = ((int16_t)uart_sbus_rx_buf[13] >> 3 | ((int16_t)uart_sbus_rx_buf[14] << 5 )) & 0x07FF;
//		*(pbuf+10) = ((int16_t)uart_sbus_rx_buf[14] >> 6 | ((int16_t)uart_sbus_rx_buf[15] << 2 )  | (int16_t)uart_sbus_rx_buf[16] << 10 ) & 0x07FF;
//		*(pbuf+11) = ((int16_t)uart_sbus_rx_buf[16] >> 1 | ((int16_t)uart_sbus_rx_buf[17] << 7 )) & 0x07FF;
//		*(pbuf+12) = ((int16_t)uart_sbus_rx_buf[17] >> 4 | ((int16_t)uart_sbus_rx_buf[18] << 4 )) & 0x07FF;
//		*(pbuf+13) = ((int16_t)uart_sbus_rx_buf[18] >> 7 | ((int16_t)uart_sbus_rx_buf[19] << 1 )  | (int16_t)uart_sbus_rx_buf[20] <<  9 ) & 0x07FF;
//		*(pbuf+14) = ((int16_t)uart_sbus_rx_buf[20] >> 2 | ((int16_t)uart_sbus_rx_buf[21] << 6 )) & 0x07FF;
//		*(pbuf+15) = ((int16_t)uart_sbus_rx_buf[21] >> 5 | ((int16_t)uart_sbus_rx_buf[22] << 3 )) & 0x07FF;
		
		uart_sbus_rx_ok = 0;
		return 1;
	}
	else
	{
		return 0;
	}	
}


