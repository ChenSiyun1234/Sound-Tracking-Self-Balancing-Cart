/* ax_uart2.c - HC-05 Bluetooth on USART2 (PA2=TX, PA3=RX).
 *
 * Simple character-command link: the RX interrupt drops each received byte into a
 * small ring buffer; AX_UART2_GetCmd() pops one. The command set (F/B/L/R/S /
 * O / E / Q ...) is interpreted in ax_control.c. (The OpenCTR X-protocol frame
 * parser + live-PID-tuning path was replaced by this for the simple-app build.)
 */
#include "ax_uart2.h"

#define U2_RXBUF 16                      /* power-of-two-ish small ring buffer */
static volatile uint8_t u2_buf[U2_RXBUF];
static volatile uint8_t u2_head = 0;     /* ISR writes here */
static volatile uint8_t u2_tail = 0;     /* GetCmd reads here */

void AX_UART2_Init(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   /* HC-05 on USART2: PA2=TX, PA3=RX */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

void USART2_IRQHandler(void)
{
	uint8_t res, next;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		res = (uint8_t)USART_ReceiveData(USART2);

		/* Drop NUL: GetCmd() returns 0 to mean "empty", and no command is 0x00,
		   so never buffering a NUL keeps that contract unambiguous. */
		if (res != 0)
		{
			next = (uint8_t)((u2_head + 1) % U2_RXBUF);
			if (next != u2_tail)          /* drop the byte if the buffer is full */
			{
				u2_buf[u2_head] = res;
				u2_head = next;
			}
		}
	}
}

uint8_t AX_UART2_GetCmd(void)
{
	uint8_t c;

	if (u2_head == u2_tail)
		return 0;                         /* empty */

	c = u2_buf[u2_tail];
	u2_tail = (uint8_t)((u2_tail + 1) % U2_RXBUF);
	return c;
}
