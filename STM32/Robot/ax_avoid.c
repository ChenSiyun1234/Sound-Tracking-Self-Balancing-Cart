/* ax_avoid.c - ultrasonic obstacle-avoidance state machine (see ax_avoid.h).
 *
 * Flow:  IDLE --(obstacle < TRIGGER)--> BRAKE --> SCAN1 (rotate one way) -->
 *        SCAN2 (rotate back through the front arc) --> TURN (face the clearest
 *        heading) --> DONE (settle) --> IDLE.
 *
 * Heading comes from the DMP yaw (ax_angle_data[2], 0.01 deg). The two scan
 * phases are bounded by ANGLE using abs/sign-safe tests, so they can't deadlock
 * if the vw->yaw sign is unknown; the sign is then read off the (now large)
 * end-of-SCAN1 heading and used to steer SCAN2 + the proportional turn-back.
 *
 * This module never touches the motors directly - it only writes ax_robot_vx /
 * ax_robot_vw, which the balance loop realises while keeping the cart upright.
 */
#include "ax_avoid.h"
#include "ax_robot.h"

/* ---- tunables ---- */
#define AV_TRIGGER_CM    25       /* obstacle closer than this (cm) starts a maneuver */
#define AV_TURN_VW       50       /* max yaw command while scanning / turning (~deg/s) */
#define AV_SCAN_RANGE    60.0f    /* sweep +/- this many deg from the entry heading */
#define AV_SCAN_CAP      120      /* per-phase ticks cap (20 ms each) if it can't rotate */
#define AV_SETTLE_TICKS  15       /* brake / done settle (~0.3 s) */
#define AV_TURN_TIMEOUT  150      /* give up turning after ~3 s */
#define AV_TURN_KP       2.0f     /* proportional gain: vw = KP * heading error (deg) */
#define AV_TOL_DEG       8.0f     /* "reached the heading" tolerance */
#define AV_DONE_CONFIRM  5        /* consecutive in-band ticks before declaring DONE */
#define AV_FAR_CM        400      /* no-echo (0) = open space = this much clearance */

static uint8_t  s_enable = 0;
static uint8_t  s_state  = AVOID_OFF;
static uint16_t s_timer  = 0;
static uint16_t s_confirm = 0;
static uint8_t  s_armed  = 1;      /* re-trigger guard: front must clear before re-arming */
static float    s_yaw0   = 0.0f;   /* heading when the maneuver started */
static float    s_best_rel = 0.0f; /* clearest heading, relative to s_yaw0 */
static uint16_t s_best_clr = 0;    /* best clearance seen (cm; no-echo mapped to AV_FAR_CM) */
static int8_t   s_yaw_sign = 1;    /* +1 if +vw increases (yaw - yaw0), else -1 */

static float yaw_deg(void)            { return ax_angle_data[2] * 0.01f; }
static float ang_norm(float a)        { while (a > 180.0f) a -= 360.0f; while (a < -180.0f) a += 360.0f; return a; }
static uint16_t clearance(uint16_t d) { return (d == 0) ? AV_FAR_CM : d; }   /* 0 = no echo = open */

void AX_AVOID_SetEnable(uint8_t en)
{
	s_enable = en ? 1 : 0;
	if (!s_enable)
	{
		s_state = AVOID_OFF;
	}
	else if (s_state == AVOID_OFF)
	{
		s_state = AVOID_IDLE;
		s_armed = 1;
	}
}

void        AX_AVOID_Toggle(void)    { AX_AVOID_SetEnable(!s_enable); }
uint8_t     AX_AVOID_IsEnabled(void) { return s_enable; }
uint8_t     AX_AVOID_IsActive(void)  { return (s_state > AVOID_IDLE); }
uint8_t     AX_AVOID_GetState(void)  { return s_state; }

const char* AX_AVOID_StateStr(void)
{
	switch (s_state)
	{
		case AVOID_OFF:   return "OFF ";
		case AVOID_IDLE:  return "IDLE";
		case AVOID_BRAKE: return "STOP";
		case AVOID_SCAN1: return "SCN1";
		case AVOID_SCAN2: return "SCN2";
		case AVOID_TURN:  return "TURN";
		case AVOID_DONE:  return "DONE";
		default:          return "??? ";
	}
}

void AX_AVOID_Update(void)
{
	uint16_t d, clr;
	float rel, err, v;

	if (!s_enable)
		return;

	d   = ax_ultra_distance;
	clr = clearance(d);

	switch (s_state)
	{
		case AVOID_IDLE:
			/* re-arm only once the front is clear, so we don't re-trigger forever
			   when boxed in (cart just waits facing the clearest heading) */
			if (d == 0 || d >= AV_TRIGGER_CM)
				s_armed = 1;

			if (s_armed && d != 0 && d < AV_TRIGGER_CM)
			{
				ax_robot_vx = 0;
				ax_robot_vw = 0;
				s_yaw0 = yaw_deg();
				s_best_rel = 0.0f;
				s_best_clr = 0;
				s_armed = 0;
				s_timer = 0;
				s_state = AVOID_BRAKE;
			}
			break;

		case AVOID_BRAKE:
			ax_robot_vx = 0;
			ax_robot_vw = 0;
			if (++s_timer >= AV_SETTLE_TICKS)
			{
				s_timer = 0;
				s_state = AVOID_SCAN1;
			}
			break;

		case AVOID_SCAN1:
			/* rotate one way until we've turned ~SCAN_RANGE (abs test = sign-safe) */
			ax_robot_vx = 0;
			ax_robot_vw = AV_TURN_VW;
			rel = ang_norm(yaw_deg() - s_yaw0);
			if (clr > s_best_clr) { s_best_clr = clr; s_best_rel = rel; }
			if (rel >= AV_SCAN_RANGE || rel <= -AV_SCAN_RANGE || ++s_timer >= AV_SCAN_CAP)
			{
				s_yaw_sign = (rel >= 0.0f) ? 1 : -1;   /* now |rel| is large -> robust */
				s_timer = 0;
				s_state = AVOID_SCAN2;
			}
			break;

		case AVOID_SCAN2:
			/* reverse and sweep to the opposite extreme (covers the full front arc) */
			ax_robot_vx = 0;
			ax_robot_vw = -AV_TURN_VW;
			rel = ang_norm(yaw_deg() - s_yaw0);
			if (clr > s_best_clr) { s_best_clr = clr; s_best_rel = rel; }
			if ((s_yaw_sign * rel) <= -AV_SCAN_RANGE || ++s_timer >= (2 * AV_SCAN_CAP))
			{
				ax_robot_vw = 0;
				s_timer = 0;
				s_confirm = 0;
				s_state = AVOID_TURN;
			}
			break;

		case AVOID_TURN:
			/* ease toward s_best_rel with a proportional yaw command (sign auto-corrected) */
			ax_robot_vx = 0;
			rel = ang_norm(yaw_deg() - s_yaw0);
			err = ang_norm(s_best_rel - rel);
			v = s_yaw_sign * AV_TURN_KP * err;
			if (v >  AV_TURN_VW) v =  AV_TURN_VW;
			else if (v < -AV_TURN_VW) v = -AV_TURN_VW;
			ax_robot_vw = (int16_t)v;

			if (err < AV_TOL_DEG && err > -AV_TOL_DEG) s_confirm++;
			else                                       s_confirm = 0;

			/* short-circuit: if confirmed, the timeout counter is not touched */
			if (s_confirm >= AV_DONE_CONFIRM || ++s_timer >= AV_TURN_TIMEOUT)
			{
				ax_robot_vw = 0;
				s_timer = 0;
				s_confirm = 0;
				s_state = AVOID_DONE;
			}
			break;

		case AVOID_DONE:
			ax_robot_vx = 0;
			ax_robot_vw = 0;
			if (++s_timer >= AV_SETTLE_TICKS)
			{
				s_timer = 0;
				s_state = AVOID_IDLE;     /* now facing the clearest way; resume monitoring */
			}
			break;

		default:
			s_state = s_enable ? AVOID_IDLE : AVOID_OFF;
			break;
	}
}
