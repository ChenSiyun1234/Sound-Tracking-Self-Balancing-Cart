/* ax_uart1.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_uart1.h"
#include <stdio.h>

#include "ax_robot.h"

static uint8_t uart1_rx_con=0;
static uint8_t uart1_rx_checksum;
static uint8_t uart1_rx_buf[40];
static uint8_t uart1_tx_buf[40];


void AX_UART1_Init(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART1, ENABLE);
}


void USART1_IRQHandler(void)
{
	uint8_t Res;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		Res =USART_ReceiveData(USART1);	
		
		if(uart1_rx_con < 3)
		{
			if(uart1_rx_con == 0)
			{
				if(Res == 0xAA)
				{
					uart1_rx_buf[0] = Res;
					uart1_rx_con = 1;					
				}
				else
				{
					
				}
			}else if(uart1_rx_con == 1)
			{
				if(Res == 0x55)
				{
					uart1_rx_buf[1] = Res;
					uart1_rx_con = 2;
				}
				else
				{
					uart1_rx_con = 0;						
				}				
			}
			else
			{
				uart1_rx_buf[2] = Res;
				uart1_rx_con = 3;
				uart1_rx_checksum = (0xAA+0x55) + Res;
			}
		}
		else
		{
			if(uart1_rx_con < (uart1_rx_buf[2]-1) )
			{
				uart1_rx_buf[uart1_rx_con] = Res;
				uart1_rx_con++;
				uart1_rx_checksum = uart1_rx_checksum + Res;					
			}
			else
			{
				uart1_rx_con = 0;	
				
				if( Res == uart1_rx_checksum )
				{	

					if(uart1_rx_buf[3] == ID_URX_VEL)
					{
						ax_robot_vx = (int16_t)((uart1_rx_buf[4]<<8) | uart1_rx_buf[5]);
						ax_robot_vw = (int16_t)((uart1_rx_buf[8]<<8) | uart1_rx_buf[9]);
						
						ax_robot_vw = ax_robot_vw*((180/PI)*0.001f);
						
						ax_control_mode = CTL_ROS;
					}
					else
					{
						if(uart1_rx_buf[3] == ID_URX_BLC)
						{
							ax_balance_kp = (int16_t)((uart1_rx_buf[4]<<8) | uart1_rx_buf[5]);
							ax_balance_kd = (int16_t)((uart1_rx_buf[6]<<8) | uart1_rx_buf[7]);
						}
						
						if(uart1_rx_buf[3] == ID_URX_BLV)
						{
							ax_velocity_kp = (int16_t)((uart1_rx_buf[4]<<8) | uart1_rx_buf[5]);
							ax_velocity_ki = (int16_t)((uart1_rx_buf[6]<<8) | uart1_rx_buf[7]);
						}						
						
						if(uart1_rx_buf[3] == ID_URX_BLT)
						{
							ax_turn_kp = (int16_t)((uart1_rx_buf[4]<<8) | uart1_rx_buf[5]);
							ax_turn_kd = (int16_t)((uart1_rx_buf[6]<<8) | uart1_rx_buf[7]);
						}							
						
						ax_beep_ring = BEEP_SHORT;
					}	
				}
			}
		}
	} 
}



void AX_UART1_SendPacket(uint8_t *pbuf, uint8_t len, uint8_t num)
{
	uint8_t i,cnt;	
  uint8_t tx_checksum = 0;
	
	if(len <= 32)
	{

		uart1_tx_buf[0] = 0xAA;
		uart1_tx_buf[1] = 0x55;    //
		uart1_tx_buf[2] = len+5;
		uart1_tx_buf[3] = num;
		
		for(i=0; i<len; i++)
		{
			uart1_tx_buf[4+i] = *(pbuf+i);
		}
		
	
		cnt = 4+len;
		for(i=0; i<cnt; i++)
		{
			tx_checksum = tx_checksum + uart1_tx_buf[i];
		}
		uart1_tx_buf[i] = tx_checksum;
		
		
	
		cnt = 5+len;
		
		for(i=0; i<cnt; i++)
		{
			USART_SendData(USART1, uart1_tx_buf[i]);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC) != SET);
		}
	}
}



int fputc(int ch, FILE *f)
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
	USART_SendData(USART1, (uint8_t) ch);

	/* Loop until the end of transmission */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}

	return ch;
}


int fgetc(FILE *f)
{

	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
	{}

	return (int)USART_ReceiveData(USART1);
}


