/* ax_uart4.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_uart4.h"
#include <stdio.h>
#include "ax_robot.h"

static uint8_t uart4_rx_ok = 0;
static uint8_t uart4_rx_con=0;
static uint8_t uart4_rx_checksum;
static uint8_t uart4_rx_buf[40];
static uint8_t uart4_tx_buf[40];


void AX_UART4_Init(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);

	USART_Cmd(UART4, ENABLE);
}


void UART4_IRQHandler(void)                	
{
	uint8_t Res;
	
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
	{
	    //printf("Get Data!\r\n");
		Res =USART_ReceiveData(UART4);	
	
		if(uart4_rx_con < 3)
		{
			if(uart4_rx_con == 0)
			{
				if(Res == 0xAA)
				{
					uart4_rx_buf[0] = Res;
					uart4_rx_con = 1;					
				}
				else
				{
					
				}
			}else if(uart4_rx_con == 1)
			{
				if(Res == 0x55)
				{
					uart4_rx_buf[1] = Res;
					uart4_rx_con = 2;
				}
				else
				{
					uart4_rx_con = 0;						
				}				
			}
			else
			{
				uart4_rx_buf[2] = Res;
				uart4_rx_con = 3;
				uart4_rx_checksum = (0xAA+0x55) + Res;
			}
		}
		else
		{
			if(uart4_rx_con < (uart4_rx_buf[2]-1) )
			{
				uart4_rx_buf[uart4_rx_con] = Res;
				uart4_rx_con++;
				uart4_rx_checksum = uart4_rx_checksum + Res;					
			}
			else
			{
				
				uart4_rx_con = 0;	
				
				if( Res == uart4_rx_checksum )
				{	
//					ax_control_mode = CTL_ROS;
//					
//					if(uart4_rx_buf[3] == ID_URX_VEL)
//					{
//						R_Vel.TG_IX = (int16_t)((uart4_rx_buf[4]<<8) | uart4_rx_buf[5]);
//						R_Vel.TG_IY = (int16_t)((uart4_rx_buf[6]<<8) | uart4_rx_buf[7]);
//						R_Vel.TG_IW = (int16_t)((uart4_rx_buf[8]<<8) | uart4_rx_buf[9]);
//					}
//					else
//					{
//						if(uart4_rx_buf[3] == ID_URX_ANGLE)
//						{
//							ax_joint_angle[0] = (int16_t)((uart4_rx_buf[4]<<8) | uart4_rx_buf[5]);
//							ax_joint_angle[1] = (int16_t)((uart4_rx_buf[6]<<8) | uart4_rx_buf[7]);
//						}				
//					}		
				}	
				
			}
		}
			
      USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
	} 
}


uint8_t AX_UART4_GetData(uint8_t *pbuf)
{
	uint8_t cnt,i;
	
	if(uart4_rx_ok != 0)
	{
		cnt = uart4_rx_buf[2]-4;
		
		for(i=0; i<cnt; i++)
		{
			*(pbuf+i) = uart4_rx_buf[3+i];
		}
		
		uart4_rx_ok = 0;
		return cnt;
	}
	else
	{
		return 0;
	}	
}


void AX_UART4_SendPacket(uint8_t *pbuf, uint8_t len, uint8_t num)
{
	uint8_t i,cnt;	
    uint8_t tx_checksum = 0;
	
	if(len <= 50)
	{

		uart4_tx_buf[0] = 0xAA;
		uart4_tx_buf[1] = 0x55;    //
		uart4_tx_buf[2] = len+5;
		uart4_tx_buf[3] = num;
		
		for(i=0; i<len; i++)
		{
			uart4_tx_buf[4+i] = *(pbuf+i);
		}
		
	
		cnt = 4+len;
		for(i=0; i<cnt; i++)
		{
			tx_checksum = tx_checksum + uart4_tx_buf[i];
		}
		uart4_tx_buf[i] = tx_checksum;
		
		
	
		cnt = 5+len;
		
		for(i=0; i<cnt; i++)
		{
			USART_SendData(UART4, uart4_tx_buf[i]);
			while(USART_GetFlagStatus(UART4,USART_FLAG_TC) != SET);
		}	
	}
}




