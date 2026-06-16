/* ax_robot.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AX_ROBOT_H
#define __AX_ROBOT_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

#include "ax_ursc.h"        //HC-SR04 ultrasonic ranging

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include "ax_sys.h"
#include "ax_delay.h"
#include "ax_led.h"
#include "ax_motor.h"
#include "ax_encoder.h"

#include "ax_uart1.h"
#include "ax_uart2.h"

#include "ax_mpu6050.h"
#include "ax_mpu6050_dmp.h"

#include "OLED.h"          //JiangXie SSD1306 I2C OLED (GND=PB6, VCC=PB7, SCL=PB8, SDA=PB9)

#define  PI           3.1416
#define  SQRT3        1.732
#define  PID_RATE     100

#define  VBAT_40P    710
#define  VBAT_20P    675
#define  VBAT_10P    656

#define  ROBOT_FW_VER   "V1.00"

#define ROBOT_B680


#define  WHEEL_DIAMETER	      0.066
#define  WHEEL_BASE           0.167
#define  WHEEL_RESOLUTION     2496.0
#define  WHEEL_SCALE          (PI*WHEEL_DIAMETER*PID_RATE/WHEEL_RESOLUTION)


#define  ID_UTX_ROS     0x10
#define  ID_UTX_DBG     0x11

#define  ID_URX_VEL     0x50
#define  ID_URX_BLC     0x02
#define  ID_URX_BLV     0x03
#define  ID_URX_BLT     0x04

#define  ID_BLERX_CM    0x30
#define  ID_BLERX_YG    0x31
#define  ID_BLERX_SB    0x32
#define  ID_BLERX_ZL    0x33
#define  ID_BLERX_TK    0x34
#define  ID_BLERX_AM    0x3A
#define  ID_BLERX_LG    0x41
#define  ID_BLERX_LS    0x42

#define  LEFFECT1    0x01
#define  LEFFECT2    0x02
#define  LEFFECT3    0x03
#define  LEFFECT4    0x04
#define  LEFFECT5    0x05
#define  LEFFECT6    0x06

#define  CTL_PS2    0x01
#define  CTL_APP    0x02
#define  CTL_RMS    0x03
#define  CTL_ROS    0x04

#define  CTL_FN1    0x11
#define  CTL_FN2    0x12
#define  CTL_FN3    0x13
#define  CTL_FN4    0x14
#define  CTL_FN5    0x15
 
#define  CTL_FN_MAX    CTL_FN5

#define  BEEP_SHORT   0x01
#define  BEEP_LONG    0x02

#define JOINTA_UP_LIMIT    900
#define JOINTA_LOW_LIMIT  -900
#define JOINTB_UP_LIMIT    900
#define JOINTB_LOW_LIMIT  -900

#define JOINTA_ANGLE_OFFSET   (0)
#define JOINTB_ANGLE_OFFSET   (0)

extern int16_t ax_robot_vx;
extern int16_t ax_robot_vw;
extern uint8_t ax_robot_move_enable;
extern uint8_t ax_velocity_enable;   /* 0 = velocity loop off (BT 'V' toggles) */
extern uint8_t ax_turn_enable;       /* 0 = turn loop off     (BT 'T' toggles) */
extern int8_t  ax_vel_sign;          /* +1/-1 velocity-loop polarity (BT 'Y' flips) */
extern uint8_t ax_midcal_enable;     /* 0 = auto-midpoint off (BT 'C' toggles) */
extern int16_t ax_deadzone;          /* motor dead-zone compensation, PWM counts (BT '5') */
extern volatile uint8_t g_cfg_state; /* 0=defaults 1=loaded-from-flash 2=save-OK 3=save-FAILED (OLED) */
extern uint8_t ax_disp_mode;         /* OLED view: 0=RUN, 1=TUNE (BT 'D') */
extern uint8_t ax_control_mode;

extern int16_t ax_acc_data[3];
extern int16_t ax_gyro_data[3];
extern int16_t ax_angle_data[3];

extern uint8_t ax_beep_ring;   /* mode-change chirp flag, written by the USART2 ISR */

extern double ax_wheel_vel_l;
extern double ax_wheel_vel_r;

extern int16_t  ax_middle_angle;  

extern int16_t  ax_balance_angle; 
extern int16_t  ax_balance_gyro;  
extern int16_t  ax_balance_out;   
extern int16_t  ax_balance_kp;
extern int16_t  ax_balance_kd;	

extern int16_t  ax_velocity;  
extern int16_t  ax_velocity_out;	
extern int16_t  ax_velocity_kp;	
extern int16_t  ax_velocity_ki;	   

extern int16_t  ax_turn_gyro;
extern int16_t  ax_turn_out;
extern int16_t  ax_turn_kp;
extern int16_t  ax_turn_kd;

extern TaskHandle_t Control_Task_Handle;
extern TaskHandle_t Disp_Task_Handle;

//Ultrasonic distance, cm (0 = no reading)
extern uint16_t ax_ultra_distance;

extern volatile uint32_t g_balance_isr_cnt;   /* DEBUG: balance-ISR (data-ready) rate counter */
extern volatile uint32_t g_dmp_data_cnt;      /* DEBUG: fresh DMP quaternion sample counter */

#endif


