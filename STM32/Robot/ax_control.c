/* ax_control.c - HC-05 Bluetooth: single-character command set (drive + live PID tuning).
 *
 * Send from a Bluetooth serial app (e.g. "Serial Bluetooth Terminal"), case-insensitive;
 * any other byte (CR/LF, etc.) is ignored.
 *
 *   DRIVE:  F=fwd  B=back  L=left  R=right  S=stop      O=toggle obstacle-avoidance
 *   LOOPS:  V=velocity loop on/off   T=turn loop on/off   Y=flip velocity-loop sign
 *           D=toggle OLED RUN/TUNE view   C=toggle auto-midpoint
 *   TUNE :  1=balance Kp  2=balance Kd  3=velocity Kp  4=velocity Ki  5=motor deadzone
 *           + / - = step the selected gain up / down
 *           M=capture current angle as the balance midpoint
 *   SAVE :  W=save ALL tuned values to flash (gains+midpoint+deadzone+vel-sign)   Z=erase
 *
 * Drive commands are LATCHED and ignored while an avoidance maneuver runs. Commands
 * only set targets / gains; the balance loop keeps the cart upright. (No "kill
 * balance" command on purpose - that just drops the cart; use the power switch.)
 */

/* Includes ------------------------------------------------------------------*/
#include "ax_control.h"
#include "ax_robot.h"
#include "ax_avoid.h"
#include "ax_cfg.h"

#define DRIVE_SPEED   250   /* forward/back target, mm/s (velocity loop clamps to +/-700) */
#define TURN_SPEED     60   /* left/right turn target (~deg/s) */

void AX_CTL_Cmd(void)
{
	static uint8_t sel = 0;                  /* which gain +/- adjusts: 0..4 */
	int16_t      *gain[5] = { &ax_balance_kp, &ax_balance_kd, &ax_velocity_kp, &ax_velocity_ki, &ax_deadzone };
	const int16_t step[5] = { 50, 5, 200, 100, 10 };
	const int16_t gmax[5] = { 9999, 999, 20000, 20000, 2000 };   /* caps match the OLED field widths (Kp 4-digit,
	                                                                Kd 3-digit, vKp/vKi 5-digit, Z 4-digit) so no display truncation; all < int16 max */
	uint8_t c;

	while ((c = AX_UART2_GetCmd()) != 0)
	{
		if (c >= 'a' && c <= 'z')
			c = (uint8_t)(c - 32);          /* to upper-case */

		switch (c)
		{
			/* --- drive --- */
			case 'F': if (!AX_AVOID_IsActive()) ax_robot_vx =  DRIVE_SPEED; break;
			case 'B': if (!AX_AVOID_IsActive()) ax_robot_vx = -DRIVE_SPEED; break;
			case 'L': if (!AX_AVOID_IsActive()) ax_robot_vw =  TURN_SPEED;  break;
			case 'R': if (!AX_AVOID_IsActive()) ax_robot_vw = -TURN_SPEED;  break;
			case 'S': ax_robot_vx = 0; ax_robot_vw = 0;                     break;
			case 'O': AX_AVOID_Toggle();                                    break;

			/* --- live PID tuning --- */
			case 'V': ax_velocity_enable = !ax_velocity_enable;    break;   /* velocity loop on/off */
			case 'T': ax_turn_enable     = !ax_turn_enable;        break;   /* turn loop on/off */
			case 'Y': ax_vel_sign        = (int8_t)-ax_vel_sign;   break;   /* flip velocity sign live (brake vs assist) */
			case '1': sel = 0; break;
			case '2': sel = 1; break;
			case '3': sel = 2; break;
			case '4': sel = 3; break;
			case '5': sel = 4; break;                                       /* select motor deadzone */
			case '+': case '=':   /* clamp to gmax so a gain can never overflow int16 and wrap negative */
				*gain[sel] = (*gain[sel] <= (int16_t)(gmax[sel] - step[sel])) ? (int16_t)(*gain[sel] + step[sel]) : gmax[sel];
				break;
			case '-': case '_':   /* floor at 0 - this also RECOVERS a value that already wrapped negative */
				*gain[sel] = (*gain[sel] >= step[sel]) ? (int16_t)(*gain[sel] - step[sel]) : 0;
				break;
			case 'M': ax_middle_angle = ax_balance_angle;          break;   /* capture balance midpoint */
			case 'C': ax_midcal_enable = !ax_midcal_enable;        break;   /* auto-midpoint on/off */
			case 'D': ax_disp_mode     = !ax_disp_mode;            break;   /* OLED RUN <-> TUNE view */

			/* --- persist to flash --- */
			case 'W': AX_CFG_Save();                               break;   /* save gains+midpoint (hold the cart; ~tens of ms pause) */
			case 'Z': AX_CFG_Erase();                              break;   /* erase saved config -> next boot uses code defaults */

			default:  break;                                                /* ignore CR/LF/unknown */
		}
	}
}
