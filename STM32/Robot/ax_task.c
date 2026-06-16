/* ax_task.c - FreeRTOS tasks for the STM32F103C8T6 balance cart.
 *
 * The self-balancing loop does NOT live here - it runs in the MPU6050
 * data-ready interrupt (EXTI9_5_IRQHandler in ax_balance.c) at a hardware-timed
 * 100 Hz, so it keeps balancing regardless of task scheduling. These tasks are
 * the slower "supervisor" layer:
 *
 *   Control_Task   - HC-05 commands -> vx/vw targets + obstacle avoidance
 *   Ultra_Task     - ping the HC-SR04 and publish the distance
 *   Disp_Task      - update the OLED
 *   Heartbeat_Task - blink the on-board PC13 LED to show the firmware is alive
 */

/* Includes ------------------------------------------------------------------*/
#include "ax_task.h"
#include "ax_robot.h"

#include "ax_control.h"
#include "ax_avoid.h"


/* Remote control + obstacle avoidance. Runs every 20 ms (AV_TICK in ax_avoid). */
void Control_Task(void* parameter)
{
	while(1)
	{
		AX_CTL_Cmd();        /* apply HC-05 commands (drive / toggle avoid / arm) */

		AX_AVOID_Update();   /* if avoidance is on + triggered, it drives vx/vw */

		/* Basic obstacle stop (always on, any mode): block forward when too close */
		if(ax_ultra_distance != 0 && ax_ultra_distance < 20 && ax_robot_vx > 0)
			ax_robot_vx = 0;

		/* Serial PLOT stream over USART1 @230400 (~50 Hz): 6 numbers/line for a PC
		   serial plotter (Arduino Serial Plotter / SerialPlot).
		   ch1 = angle error from midpoint (0.01 deg; 0 = balanced)
		   ch2 = tilt rate (deg/s)
		   ch3 = left  wheel speed (mm/s)   ch4 = right wheel speed (mm/s)
		   ch5 = velocity-loop output       ch6 = turn-loop output (PWM counts)
		   VERIFY VELOCITY SIGN (V-only mode, vKi=0): push a wheel by hand -> ch5
		   (velocity_out) must OPPOSE the push and the motor must brake/fight you. If it
		   assists (runs away in the push direction), flip BOTH encoder signs in
		   ax_balance.c. ch3/ch4 are raw wheel speed; ch6 = turn-loop output. */
		printf("%d %d %d %d %d %d\r\n",
		       (int)(ax_balance_angle - ax_middle_angle),
		       (int)(ax_balance_gyro * 0.061f),
		       (int)(ax_wheel_vel_l * 1000),
		       (int)(ax_wheel_vel_r * 1000),
		       (int)ax_velocity_out,
		       (int)ax_turn_out);

		vTaskDelay(20);
	}
}


/* Ultrasonic ranging - ping the HC-SR04 and publish the distance (cm). */
void Ultra_Task(void* parameter)
{
	while(1)
	{
		ax_ultra_distance = (uint16_t)AX_URSC_GetDistance();  //trigger + time echo (bounded-blocking), cm
		vTaskDelay(50);                                       //~20 Hz (fresher for stop + scan)
	}
}


/* OLED status display - two views (BT 'D' toggles ax_disp_mode):
   RUN  = avoidance / distance / drive  (default for the operating phase)
   TUNE = the PID gains. */
void Disp_Task(void* parameter)
{
	OLED_Clear();

	while (1)
	{
		static uint8_t last_mode = 0xFF;
		if (ax_disp_mode != last_mode) { OLED_Clear(); last_mode = ax_disp_mode; }  /* wipe stale chars on switch */

		if (ax_disp_mode == 0)
		{
			/* ---- RUN view: avoidance + ultrasonic + remote drive ---- */
			/* L1: avoidance state (4-char) + armed/fallen */
			OLED_ShowString(1, 1, "AV:");
			OLED_ShowString(1, 4, (char *)AX_AVOID_StateStr());   /* OLED API wants char*; the string isn't modified */
			OLED_ShowString(1, 9, ax_robot_move_enable ? "ARM " : "FALL");
			OLED_ShowString(1, 13, "    ");

			/* L2: ultrasonic distance (cm) */
			OLED_ShowString(2, 1, "Dist:");
			OLED_ShowNum(2, 6, ax_ultra_distance, 4);
			OLED_ShowString(2, 10, "cm   ");

			/* L3: remote drive targets */
			OLED_ShowString(3, 1, "Vx:");
			OLED_ShowSignedNum(3, 4, ax_robot_vx, 3);
			OLED_ShowString(3, 8, " Vw:");
			OLED_ShowSignedNum(3, 12, ax_robot_vw, 3);

			/* L4: balance angle (deg) + loop state + flash status (L/S/X) */
			OLED_ShowString(4, 1, "A:");
			OLED_ShowSignedNum(4, 3, ax_balance_angle / 100, 3);
			OLED_ShowString(4, 8, ax_velocity_enable ? (ax_vel_sign > 0 ? "V+" : "V-") : "V0");
			OLED_ShowString(4, 11, ax_turn_enable ? "T1" : "T0");
			OLED_ShowString(4, 14, "  ");
			OLED_ShowString(4, 16, g_cfg_state == 1 ? "L" : g_cfg_state == 2 ? "S" : g_cfg_state == 3 ? "X" : " ");
		}
		else
		{
			/* ---- TUNE view: PID gains ---- */
			OLED_ShowString(1, 1, ax_velocity_enable ? (ax_vel_sign > 0 ? "V+" : "V-") : "V0");
			OLED_ShowString(1, 4, ax_turn_enable ? "T1" : "T0");
			OLED_ShowString(1, 7, ax_robot_move_enable ? "ARM " : "FALL");
			OLED_ShowString(1, 11, "Z:");
			OLED_ShowNum(1, 13, ax_deadzone, 4);

			OLED_ShowString(2, 1, "A:");
			OLED_ShowSignedNum(2, 3, ax_balance_angle / 100, 3);
			OLED_ShowString(2, 8, "M:");
			OLED_ShowSignedNum(2, 10, ax_middle_angle, 4);
			OLED_ShowString(2, 15, ax_midcal_enable ? "C" : " ");
			OLED_ShowString(2, 16, g_cfg_state == 1 ? "L" : g_cfg_state == 2 ? "S" : g_cfg_state == 3 ? "X" : " ");

			OLED_ShowString(3, 1, "P:");
			OLED_ShowNum(3, 3, ax_balance_kp, 4);
			OLED_ShowString(3, 8, "D:");
			OLED_ShowNum(3, 10, ax_balance_kd, 3);
			OLED_ShowString(3, 13, "   ");

			OLED_ShowString(4, 1, "vP:");
			OLED_ShowNum(4, 4, ax_velocity_kp, 5);
			OLED_ShowString(4, 9, "vI:");
			OLED_ShowNum(4, 12, ax_velocity_ki, 5);
		}

		vTaskDelay(100);
	}
}


/* Blink the on-board LED (PC13) so it's obvious the firmware is running.
   (No serial printing here - USART1 now carries the plot stream from Control_Task;
   the gains/mode are on the OLED.) */
void Heartbeat_Task(void* parameter)
{
	while (1)
	{
		AX_LED_Toggle();
		vTaskDelay(500);
	}
}
