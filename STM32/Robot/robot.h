/* robot.h - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ROBOT_H
#define __ROBOT_H

/* Includes ------------------------------------------------------------------*/	 
#include "stm32f10x.h"

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
#include "ax_beep.h"
#include "ax_vin.h"
#include "ax_key.h"

#include "ax_uart1.h"
#include "ax_uart2.h"
#include "ax_uart4.h"
#include "ax_uart5.h"

#include "ax_motor.h"
#include "ax_encoder.h"
#include "ax_servo.h"

#include "ax_mpu6050.h"

#include "ax_rgb.h"
#include "ax_sbus.h"
#include "ax_oled.h"


typedef struct  
{
	float  Wheel_RT;
	float  Wheel_TG;
	short  Motor_Pwm;
}MOTOR_Data;

typedef struct  
{
	short  I_X;
	short  I_Y;
	short  I_W;
	
	float  F_X;
	float  F_Y;
	float  F_W;
	
}ROBOT_Velocity;

typedef struct  
{
	float  Radius;
	float  Angle;
	float  RAngle;
	float  SAngle;
	
}ROBOT_Steering;

typedef struct  
{
	short  ACC_X;
	short  ACC_Y;
	short  ACC_Z;
	
	short  GYRO_X;
	short  GYRO_Y;
	short  GYRO_Z;
	
}ROBOT_IMU;


#define  PI   3.1416

#define  VBAT_40P    1065
#define  VBAT_20P    1012
#define  VBAT_10P    984

#define  PID_RATE        50

#define  R_WHEEL_BASE   0.165
#define  R_ACLE_BASE    0.160


#define  R_TURN_R_MINI  0.35

#define  WHEEL_DIAMETER	     0.0725
#define  WHEEL_RESOLUTION    1440.0

#define  WHEEL_SCALE  (PI*WHEEL_DIAMETER*PID_RATE/WHEEL_RESOLUTION) 

#define R_VX_LIMIT  1500
#define R_VY_LIMIT  1200
#define R_VW_LIMIT  6280

#define  ID_CPR2ROS_DATA    0x10
#define  ID_ROS2CRP_VEL     0x50
#define  ID_ROS2CRP_IMU     0x51
#define  ID_ROS2CRP_AKM     0x5f

extern int16_t ax_imu_acc_data[3];  
extern int16_t ax_imu_gyro_data[3];
extern int16_t ax_imu_gyro_offset[3];   

extern int16_t ax_motor_kp;  
extern int16_t ax_motor_ki;    
extern int16_t ax_motor_kd; 

extern int16_t ax_servo_offset;

extern int8_t ax_imu_calibrate_flag;

extern MOTOR_Data MOTOR_A,MOTOR_B;

extern ROBOT_Velocity  RobotV_RT,RobotV_TG;


extern TaskHandle_t Robot_Task_Handle;
extern TaskHandle_t Key_Task_Handle;
extern TaskHandle_t Bat_Task_Handle;


#endif


