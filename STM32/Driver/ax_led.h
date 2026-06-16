/* ax_led.h - on-board LED on the STM32F103C8T6 "blue pill" core board.
 *
 * The generic C8T6 board has a single user LED on PC13, wired active-LOW
 * (pin low = lit). All the colour aliases below map to that one LED so any
 * leftover Red/Green calls still build; the firmware uses it as a heartbeat.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_LED_H
#define __AX_LED_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

void AX_LED_Init(void);

/* PC13 on-board LED (active low) */
#define AX_LED_On()           GPIO_ResetBits(GPIOC, GPIO_Pin_13)
#define AX_LED_Off()          GPIO_SetBits(GPIOC, GPIO_Pin_13)
#define AX_LED_Toggle()       GPIO_WriteBit(GPIOC, GPIO_Pin_13, (BitAction)(1 - GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)))

/* Colour aliases (single physical LED) so existing Red/Green calls still build */
#define AX_LED_Red_Off()      AX_LED_Off()
#define AX_LED_Red_On()       AX_LED_On()
#define AX_LED_Red_Toggle()   AX_LED_Toggle()

#define AX_LED_Green_Off()    AX_LED_Off()
#define AX_LED_Green_On()     AX_LED_On()
#define AX_LED_Green_Toggle() AX_LED_Toggle()

#endif
