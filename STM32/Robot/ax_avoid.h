/* ax_avoid.h - ultrasonic obstacle-avoidance state machine.
 *
 * The HC-SR04 is fixed facing forward, so to "look" left/right the cart rotates
 * in place (yaw) and samples the distance vs heading, then turns toward the most
 * open direction. Throughout, the balance loop (MPU ISR) keeps the cart upright -
 * this module only sets the ax_robot_vx / ax_robot_vw targets.
 *
 * Off by default; toggled on by a Bluetooth command (see ax_control.c).
 */
#ifndef __AX_AVOID_H
#define __AX_AVOID_H

#include "stm32f10x.h"

/* state machine states (also used for the OLED label) */
#define AVOID_OFF    0
#define AVOID_IDLE   1   /* enabled, monitoring */
#define AVOID_BRAKE  2   /* obstacle seen, stopping */
#define AVOID_SCAN1  3   /* sweeping one way */
#define AVOID_SCAN2  4   /* sweeping back the other way */
#define AVOID_TURN   5   /* turning to the clearest heading */
#define AVOID_DONE   6   /* settling, then back to IDLE */

void        AX_AVOID_SetEnable(uint8_t en);
void        AX_AVOID_Toggle(void);
uint8_t     AX_AVOID_IsEnabled(void);
uint8_t     AX_AVOID_IsActive(void);     /* 1 while running a maneuver (not IDLE/OFF) */
uint8_t     AX_AVOID_GetState(void);
const char* AX_AVOID_StateStr(void);     /* 4-char label for the OLED */
void        AX_AVOID_Update(void);        /* call every Control_Task tick (~20 ms) */

#endif
