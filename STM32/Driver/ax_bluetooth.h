/**
 * ax_bluetooth.h - Bluetooth gamepad remote (HC-05/06 over UART5 RX = PD2)
 *
 * Receive-only Bluetooth serial link (SPP). A phone app's virtual gamepad, or a
 * physical gamepad bridged over Bluetooth SPP, sends framed packets carrying the
 * stick axes + buttons; the cart maps them to its target velocity / turn rate.
 *
 * Packet (X-Protocol frame, same framing as the OpenCTR UARTs):
 *     AA 55 | LEN | 35 | LX LY RX RY BTL BTH | CHK
 *       hdr    len   id   payload (6 bytes)     checksum
 *   - LEN : total frame length = payload + 5
 *   - id  : 0x35 = BT_FRAME_GAMEPAD
 *   - LX..RY : signed int8, -100..+100 (0 = centred)
 *   - BTL/BTH: 16-bit button bitmask, low byte first
 *   - CHK : 8-bit sum of all preceding bytes
 *
 * Wiring: HC-05/06 TX -> STM32 PD2 (UART5_RX), GND-GND, VCC-5V. The MCU does not
 * transmit for remote-only control, so UART5_TX (PC12) is left free for the
 * on-board LED.
 *
 * Transport adapted from the XTARK OpenCTR ax_uart5 driver.
 */
#ifndef __AX_BLUETOOTH_H
#define __AX_BLUETOOTH_H

#include "stm32f10x.h"

#define BT_FRAME_GAMEPAD   0x35      /* frame id for a gamepad packet */

/* Button bit positions within the 16-bit mask */
#define BT_BTN_A       0x0001
#define BT_BTN_B       0x0002
#define BT_BTN_X       0x0004
#define BT_BTN_Y       0x0008
#define BT_BTN_L1      0x0010
#define BT_BTN_R1      0x0020
#define BT_BTN_START   0x0040
#define BT_BTN_SELECT  0x0080

/* Decoded gamepad state */
typedef struct
{
    uint8_t  connected;     /* set to 1 once the first valid packet arrives */
    int8_t   lx, ly;        /* left  stick, -100..+100 */
    int8_t   rx, ry;        /* right stick, -100..+100 */
    uint16_t buttons;       /* BT_BTN_* bitmask */
} BT_Gamepad_TypeDef;

void    AX_BT_Init(void);                       /* bring up UART5 RX (PD2) @ 9600 */
uint8_t AX_BT_Update(void);                     /* poll link; returns 1 if a fresh gamepad packet was decoded */
const BT_Gamepad_TypeDef* AX_BT_GetGamepad(void);
uint8_t AX_BT_IsConnected(void);

#endif
