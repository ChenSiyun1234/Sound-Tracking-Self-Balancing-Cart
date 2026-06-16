/* ax_function.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Includes ------------------------------------------------------------------*/
#include "ax_function.h"
#include "ax_robot.h"



#define LS_AVOID_Distance        400
#define LS_AVOID_Distance_min    250
#define LS_AVOID_Speed           300
#define LS_AVOID_Turn            100


void AX_FUN_List1(void)
{
	uint8_t i;
	
	uint16_t min_distance = 65000;
	uint16_t min_angle = 0;
	uint16_t min_distance_cnt = 0;
	
	for(i = 0; i < 250; i++)
	{
		if((ax_ls_point[i].angle>30000) || (ax_ls_point[i].angle<6000))  
		{	
			if((ax_ls_point[i].distance >0)  && (ax_ls_point[i].distance < LS_AVOID_Distance))
			{
				if(ax_ls_point[i].distance < min_distance && ax_ls_point[i].distance>100)
				{
					min_distance = ax_ls_point[i].distance;
					min_angle = ax_ls_point[i].angle;
				}
				
				if(ax_ls_point[i].distance < LS_AVOID_Distance_min)
					min_distance_cnt++;
			}
		}
	}
	
	//printf("@%d %d \r\n",min_angle, min_distance);
	
	ax_robot_vx = LS_AVOID_Speed;
	
	if(min_distance < LS_AVOID_Distance)
	{
		if(min_distance_cnt > 3)
		{
			ax_robot_vx = -LS_AVOID_Speed;
			ax_robot_vw = 0;
		}
		else 
		{
			if(min_angle < 6000)
			{
				ax_robot_vw = LS_AVOID_Turn;
			}
			
			if(min_angle > 30000)
			{
				ax_robot_vw = -LS_AVOID_Turn;
			}
		}
	}
	else
	{
		ax_robot_vw = 0;
	}

	vTaskDelay(100); 
}






#define LS_ALARM_DistanceMin 150
#define LS_ALARM_DistanceMax 500


void AX_FUN_List2(void)
{
	uint8_t i;
	
	uint16_t min_distance = 65000;
	uint16_t min_angle = 0;
    int16_t  follow_angle = 0;
	
	for(i = 0; i < 250; i++)
	{
		if((ax_ls_point[i].angle>30000) || (ax_ls_point[i].angle<6000))  
		{	
			if((ax_ls_point[i].distance >LS_ALARM_DistanceMin) && (ax_ls_point[i].distance < LS_ALARM_DistanceMax))
			{	
				if(ax_ls_point[i].distance < min_distance)
				{
					min_distance = ax_ls_point[i].distance;
					min_angle = ax_ls_point[i].angle;
				}
			}
		}
	}
	
	if(min_angle > 18000)
		follow_angle = (min_angle - 36000)/100.0;	
	else
		follow_angle = min_angle/100.0;
	
	ax_robot_vx = 0;
	
	if(min_angle != 0)
	{
		AX_BEEP_On();
		
		ax_robot_vw = -3*follow_angle;
	}
	else
	{
		AX_BEEP_Off();
		
		ax_robot_vw = 0;
	}
	
	//printf("@%d %d \r\n",follow_angle, ax_robot_vw);

	vTaskDelay(100); 
}





#define LS_FOLLOW_DistanceMin  200
#define LS_FOLLOW_DistanceMax  1500
#define LS_FOLLOW_DistanceKeep 350

float ls_fangle_kp = 3.0f;
float ls_fangle_kd = 1.0f;
float ls_fdistance_kp = 1.0f;
float ls_fdistance_kd = 0.5f;

int16_t FOLLOW_Angle_PIDControl(float target, float current);
int16_t FOLLOW_Distance_PIDControl(float target, float current);


void AX_FUN_List3(void)
{
	uint8_t i;
	
	uint16_t min_distance = 65000;
	uint16_t min_angle = 0;
	
	static float follow_angle = 0;
	static float follow_distance = 0;
	
	for(i = 0; i < 250; i++)
	{
		if((ax_ls_point[i].angle>30000) || (ax_ls_point[i].angle<6000))  
		{	
			if((ax_ls_point[i].distance >200) && (ax_ls_point[i].distance < 1000))
			{	
				if(ax_ls_point[i].distance < min_distance)
				{
					min_distance = ax_ls_point[i].distance;
					min_angle = ax_ls_point[i].angle;
				}
			}
		}
	}
	
	if(min_angle > 18000)
		follow_angle = (min_angle - 36000)/100.0;	
	else
		follow_angle = min_angle/100.0;
		
	follow_distance = min_distance;	
	 
	//printf("@%d %d \r\n",follow_angle*100, follow_distance*100);
	
	if(min_angle != 0)
	{
		ax_robot_vx = FOLLOW_Distance_PIDControl(LS_FOLLOW_DistanceKeep, follow_distance);
		ax_robot_vw = FOLLOW_Angle_PIDControl(0, follow_angle);
	}
	else
	{
		ax_robot_vx = 0;
		ax_robot_vw = 0;
	}

	vTaskDelay(100); 
}


int16_t FOLLOW_Angle_PIDControl(float target, float current)
{
	static float bias, output, last_bias;
	
	bias = target - current;
	
	output = ls_fangle_kp*bias + ls_fangle_kd*(bias-last_bias);
	
	last_bias = bias;
	
	//printf("%f %f \r\n",bias, output);
	
	return  (output);
}


int16_t FOLLOW_Distance_PIDControl(float target, float current)
{
	static float bias, output, last_bias;
	
	bias = target - current;
	
	output = -ls_fdistance_kp*bias - ls_fdistance_kd*(bias-last_bias);
	
	last_bias = bias;
	
	//printf("@%f %f \r\n",bias, output);
	
	return  (output);
	
}




#define LS_LINE_Speed          200

float ls_line_kp = 0.2f;
float ls_line_kd = 0.1f;

int16_t LINE_PIDControl(float target, float current);


void AX_FUN_List4(void)
{
	uint8_t i;
	static uint8_t cnt=0;
	
	static uint16_t current_distance = 200; 
	static uint16_t last_distance = 200;
	
	static uint16_t target_distance = 200; 
	
	for(i = 0; i < 250; i++)
	{
		if((ax_ls_point[i].angle>7200) && (ax_ls_point[i].angle<7800))  
		{	
			if(ax_ls_point[i].distance < (target_distance + 300))
			{	
				current_distance = ax_ls_point[i].distance;			
				last_distance = current_distance;
			}
			else
			{
				current_distance = last_distance;
			}
		}
	}
	
	if(cnt > 30)
	{
		ax_robot_vx = LS_LINE_Speed;
		
		ax_robot_vw = LINE_PIDControl(target_distance, current_distance);  
		 
		//printf("@%d %d \r\n",current_distance,ax_robot_vw*10);		
	}
	else
	{
		target_distance = current_distance;
		
		cnt++;
	}
	
	vTaskDelay(100); 
}


int16_t LINE_PIDControl(float target, float current)
{
	static float bias, output, last_bias;
	
	bias = target - current;
	
	output = ls_line_kp*bias + ls_line_kd*(bias-last_bias);
	
	last_bias = bias;
	
	//printf("@%f %f \r\n",bias, output);
	
	return  (output);
	
}



 
int16_t ax_ccd_speed = 300;
int16_t ax_ccd_offset;
int16_t ax_ccd_kp = 50;
int16_t ax_ccd_kd = 20;



void AX_FUN_List5(void)
{
	static float bias,bias_last;
	float move_w=0;   

	ax_robot_vx = ax_ccd_speed;
	
	ax_ccd_offset = AX_CCD_GetOffset();
	bias = ax_ccd_offset;
	
	move_w = -ax_ccd_kp*bias*0.01f - ax_ccd_kd*(bias-bias_last)*0.01f;
	
	ax_robot_vw  =  0.01f* move_w*ax_ccd_speed;	
	
	bias_last = bias;
	
	//printf("@%d %d \r\n",ax_ccd_offset, ax_robot_vw);
	
	vTaskDelay(20); 
}



