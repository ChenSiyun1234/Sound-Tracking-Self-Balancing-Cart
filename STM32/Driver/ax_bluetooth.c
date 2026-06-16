/**
 * ax_bluetooth.c - Bluetooth gamepad remote (HC-05/06 over UART5 RX = PD2)
 *
 * Receive-only X-Protocol parser on UART5. The RXNE interrupt assembles framed
 * packets byte-by-byte; AX_BT_Update() decodes the latest gamepad frame into a
 * BT_Gamepad_TypeDef. See ax_bluetooth.h for the packet format.
 *
 * Transport adapted from the XTARK OpenCTR ax_uart5 driver.
 */
#include "ax_bluetooth.h"

/* RX frame state machine (X-Protocol: AA 55 LEN ID payload... CHK) */
static uint8_t bt_rx_con = 0;        /* byte counter / parser state */
static uint8_t bt_rx_checksum;       /* running checksum */
static uint8_t bt_rx_buf[40];        /* raw frame buffer */
static uint8_t bt_frame_ok = 0;      /* set by ISR when a full valid frame is in bt_rx_buf */

static uint8_t bt_data[40];          /* decoded frame: [0]=id, [1..]=payload */
static BT_Gamepad_TypeDef g_pad;     /* latest decoded gamepad state */

/**
 * Initialise UART5 for receive-only operation on PD2 (9600 8N1).
 * PC12 (UART5_TX) is intentionally left unconfigured so the on-board LED keeps it.
 */
void AX_BT_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);   /* PD2 = UART5_RX */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = 9600;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx;
    USART_Init(UART5, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART5, ENABLE);
}

/**
 * UART5 RX interrupt - assemble one X-Protocol frame into bt_rx_buf.
 */
void UART5_IRQHandler(void)
{
    uint8_t Res;

    if (USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(UART5);

        if (bt_rx_con < 3)               /* header + length */
        {
            if (bt_rx_con == 0)          /* header 1: 0xAA */
            {
                if (Res == 0xAA)
                {
                    bt_rx_buf[0] = Res;
                    bt_rx_con = 1;
                }
            }
            else if (bt_rx_con == 1)     /* header 2: 0x55 */
            {
                if (Res == 0x55)
                {
                    bt_rx_buf[1] = Res;
                    bt_rx_con = 2;
                }
                else
                {
                    bt_rx_con = 0;
                }
            }
            else                         /* frame length */
            {
                if (Res < 5 || Res > sizeof(bt_rx_buf))   /* reject implausible length -> no buffer overrun */
                {
                    bt_rx_con = 0;
                }
                else
                {
                    bt_rx_buf[2] = Res;
                    bt_rx_con = 3;
                    bt_rx_checksum = (0xAA + 0x55) + Res;
                }
            }
        }
        else                             /* payload + checksum */
        {
            if (bt_rx_con < (bt_rx_buf[2] - 1))
            {
                bt_rx_buf[bt_rx_con] = Res;
                bt_rx_con++;
                bt_rx_checksum += Res;
            }
            else                         /* last byte = checksum */
            {
                if (Res == bt_rx_checksum)
                {
                    bt_frame_ok = 1;     /* full valid frame ready */
                }
                bt_rx_con = 0;
            }
        }

        USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
    }
}

/**
 * Decode the most recent frame. Returns 1 if a fresh gamepad packet was applied.
 */
uint8_t AX_BT_Update(void)
{
    uint8_t cnt, i;

    if (bt_frame_ok == 0)
        return 0;

    /* guard the length byte before the unsigned subtraction below */
    if (bt_rx_buf[2] < 5 || bt_rx_buf[2] > sizeof(bt_rx_buf))
    {
        bt_frame_ok = 0;
        return 0;
    }

    /* copy [id, payload...] out of the raw buffer (payload starts at index 3) */
    cnt = bt_rx_buf[2] - 4;
    if (cnt > sizeof(bt_data))
        cnt = sizeof(bt_data);
    for (i = 0; i < cnt; i++)
        bt_data[i] = bt_rx_buf[3 + i];
    bt_frame_ok = 0;

    if (bt_data[0] == BT_FRAME_GAMEPAD && cnt >= 7)
    {
        g_pad.lx = (int8_t)bt_data[1];
        g_pad.ly = (int8_t)bt_data[2];
        g_pad.rx = (int8_t)bt_data[3];
        g_pad.ry = (int8_t)bt_data[4];
        g_pad.buttons = (uint16_t)bt_data[5] | ((uint16_t)bt_data[6] << 8);
        g_pad.connected = 1;
        return 1;
    }

    return 0;
}

const BT_Gamepad_TypeDef* AX_BT_GetGamepad(void)
{
    return &g_pad;
}

uint8_t AX_BT_IsConnected(void)
{
    return g_pad.connected;
}
