/* ax_mpu6050_dmp.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */
  

#include "ax_mpu6050_dmp.h" 

#include <stdio.h>
#include <math.h>

#include "ax_delay.h"
//#include "ax_basis.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#define DEFAULT_MPU_HZ  (100)
#define q15   32768.0f   
#define q30   1073741824.0f

short ax_gyro[3], ax_accel[3], sensors;
float ax_pitch,ax_roll,ax_yaw; 
float q0=1.0f,q1=0.0f,q2=0.0f,q3=0.0f;

static signed char gyro_orientation[9] = { 1, 0, 0,
                                           0, 1, 0,
                                           0, 0, 1};

//										   
static uint16_t inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

static  uint16_t inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;

    return scalar;
}

static void run_self_test(void)
{
    int result;
    long gyro[3], accel[3];

    result = mpu_run_self_test(gyro, accel);
	
    if (result == 0x3) 
	{
        /* Test passed. We can trust the gyro data here, so let's push it down
         * to the DMP.
         */
        float sens;
        unsigned short accel_sens;
		
        mpu_get_gyro_sens(&sens);
        gyro[0] = (long)(gyro[0] * sens);
        gyro[1] = (long)(gyro[1] * sens);
        gyro[2] = (long)(gyro[2] * sens);
        dmp_set_gyro_bias(gyro);
		
        mpu_get_accel_sens(&accel_sens);
        accel[0] *= accel_sens;
        accel[1] *= accel_sens;
        accel[2] *= accel_sens;
        dmp_set_accel_bias(accel);
    }
}


void AX_MPU6050_DMP_Init(void)
{
    u8 res=0;
    u8 tries;

	/* Retry mpu_init a few times: on a cold boot the MPU may not ACK the very
	   first attempt even after the startup settle delay in main(). */
	for(tries=0; tries<10; tries++)
	{
		if(mpu_init() == 0) break;
		AX_Delayms(100);
	}

	if(tries < 10)
	{
		res=mpu_set_sensors(INV_XYZ_GYRO|INV_XYZ_ACCEL);
		if(res) printf("mpu_set_sensor error\r\n");
		
		res=mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
		if(res) printf("mpu_configure_fifo error\r\n"); 
		
		res=mpu_set_sample_rate(DEFAULT_MPU_HZ);
		if(res) printf("mpu_set_sample_rate error\r\n");
		
		res=dmp_load_motion_driver_firmware();
		if(res) printf("dmp_load_motion_driver_firmware error\r\n"); 
		
		res=dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
		if(res) printf("dmp_set_orientation error\r\n");; 
		
		/* Balance cart needs only the quaternion + raw accel/cal-gyro. Dropped
		   DMP_FEATURE_TAP and DMP_FEATURE_ANDROID_ORIENT: those gesture detectors
		   fire EXTRA interrupts on motion (seen as ISR/s rising 180->200 while the
		   cart shook) and disrupt the quaternion FIFO read, stalling fresh data. */
		res=dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT|
		    DMP_FEATURE_SEND_RAW_ACCEL|DMP_FEATURE_SEND_CAL_GYRO|
		    DMP_FEATURE_GYRO_CAL);
		if(res) printf("dmp_enable_feature error\r\n");
		
		res=dmp_set_fifo_rate(DEFAULT_MPU_HZ);
		if(res) printf("dmp_set_fifo_rate error.\r\n");
		
		run_self_test();
		
		res=mpu_set_dmp_state(1);
		if(res) printf("mpu_set_dmp_state error.\r\n"); 
	}
}



volatile uint32_t g_dmp_data_cnt = 0;   /* DEBUG: counts fresh DMP quaternion samples */

void AX_MPU6050_DMP_GetData(int16_t *gyro, int16_t *acc, int16_t *angle)
{
	unsigned long sensor_timestamp;
	unsigned char more;
	long quat[4];
	dmp_read_fifo(gyro, acc, quat, &sensor_timestamp, &sensors, &more);

	if ( sensors & INV_WXYZ_QUAT )
	{
		g_dmp_data_cnt++;   /* DEBUG: a fresh angle sample arrived */
		 q0=quat[0] / q30;
		 q1=quat[1] / q30;
		 q2=quat[2] / q30;
		 q3=quat[3] / q30;		
		
		 ax_pitch = asin(-2 * q1 * q3 + 2 * q0* q2)* 57.3; 	//pitch
		 ax_roll = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1)* 57.3; //roll
		 ax_yaw  = atan2(2*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3) * 57.3;	//yaw
		
		 angle[0] = ax_roll*100;		
		 angle[1] = ax_pitch*100;	
		 angle[2] = ax_yaw*100;	
	}
}



