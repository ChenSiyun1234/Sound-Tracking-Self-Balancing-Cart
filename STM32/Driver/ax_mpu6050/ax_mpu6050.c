/* ax_mpu6050.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_mpu6050.h" 
#include "ax_sys.h"
#include "ax_delay.h"

/* C8T6: software I2C on PB10 (SCL) / PB11 (SDA). PB11 = port-B CRH bits [15:12] */
#define SDA_IN()  {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=(u32)8<<12;}
#define SDA_OUT() {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=(u32)3<<12;}

#define IIC_SCL    PBout(10) //SCL = PB10
#define IIC_SDA    PBout(11) //SDA = PB11
#define READ_SDA   PBin(11)

static void MPU6050_WriteRegister(uint8_t reg_address, uint8_t data);
static void MPU6050_ReadRegister(uint8_t reg_address, uint8_t *pdata, uint16_t len);



void AX_MPU6050_Init(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE );

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    IIC_SCL=1;
    IIC_SDA=1;
	
    MPU6050_WriteRegister(MPU6050_PWR_MGMT1_REG,0x00);

	MPU6050_WriteRegister(MPU6050_GYRO_CFG_REG,0x18);
    MPU6050_WriteRegister(MPU6050_ACCEL_CFG_REG,0x00);
	
	MPU6050_WriteRegister(MPU6050_INTBP_CFG_REG,0x80);
	
	MPU6050_WriteRegister(MPU6050_PWR_MGMT1_REG,0x01);
	MPU6050_WriteRegister(MPU6050_PWR_MGMT2_REG,0x00);
	
	AX_Delayms(50);
	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource5);

	EXTI_InitStructure.EXTI_Line = EXTI_Line5;//LINE
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}



void AX_MPU6050_SetAccRange(uint8_t range)
{
	MPU6050_WriteRegister(MPU6050_ACCEL_CFG_REG,range<<3);
}


void AX_MPU6050_SetGyroRange(uint8_t range)
{
	MPU6050_WriteRegister(MPU6050_GYRO_CFG_REG,range<<3);
}


void AX_MPU6050_SetGyroSmplRate(uint16_t smplrate)
{	
	if(smplrate>1000)
		smplrate = 1000;
	if(smplrate<10)
		smplrate = 10;
	
	MPU6050_WriteRegister(MPU6050_SAMPLE_RATE_REG,(uint8_t)(1000/smplrate -1));	
}


void AX_MPU6050_SetDLPF(uint8_t bandwidth)
{
	MPU6050_WriteRegister(MPU6050_CFG_REG,bandwidth);
}


float AX_MPU6050_GetTempValue(void)
{	
	uint8_t buf[2];
	int16_t tmp;

	MPU6050_ReadRegister(MPU6050_TEMP_OUTH_REG,buf,2);

	tmp = (buf[0]<<8)| buf[1];
	
	return ( 36.53f + ((double)tmp/340.0f) );	
}


void AX_MPU6050_GetAccData(int16_t *pbuf)
{	
	uint8_t buf[6];
	
	MPU6050_ReadRegister(MPU6050_ACCEL_XOUTH_REG,buf,6);
	
    pbuf[0] = (buf[0] << 8) | buf[1];
    pbuf[1] = (buf[2] << 8) | buf[3];
    pbuf[2] = (buf[4] << 8) | buf[5];	
}


void AX_MPU6050_GetGyroData(int16_t *pbuf)
{	
	uint8_t buf[6];
	
	MPU6050_ReadRegister(MPU6050_GYRO_XOUTH_REG,buf,6);
	
    pbuf[0] = (buf[0] << 8) | buf[1];
    pbuf[1] = (buf[2] << 8) | buf[3];
    pbuf[2] = (buf[4] << 8) | buf[5];	
}										



static  int IIC_Start(void)
{
	SDA_OUT();
	IIC_SDA=1;
	if(!READ_SDA)return 0;	
	IIC_SCL=1;
	AX_Delayus(1);
 	IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	if(READ_SDA)return 0;
	AX_Delayus(1);
	IIC_SCL=0;
	return 1;
}

	  
static  void IIC_Stop(void)
{
	SDA_OUT();
	IIC_SCL=0;
	IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	AX_Delayus(1);
	IIC_SCL=1; 
	IIC_SDA=1;
	AX_Delayus(1);							   	
}


static  int IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_IN();
	IIC_SDA=1;
	AX_Delayus(1);	   
	IIC_SCL=1;
	AX_Delayus(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>50)
		{
			IIC_Stop();
			return 0;
		}
	  AX_Delayus(1);
	}
	IIC_SCL=0;
	return 1;  
} 


static  void IIC_Ack(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=0;
	AX_Delayus(1);
	IIC_SCL=1;
	AX_Delayus(1);
	IIC_SCL=0;
}
	
	    
static  void IIC_NAck(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=1;
	AX_Delayus(1);
	IIC_SCL=1;
	AX_Delayus(1);
	IIC_SCL=0;
}
		  
static  void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	SDA_OUT(); 	    
    IIC_SCL=0;
    for(t=0;t<8;t++)
    {              
        IIC_SDA=(txd&0x80)>>7;
        txd<<=1; 	  
		AX_Delayus(1);   
		IIC_SCL=1;
		AX_Delayus(1); 
		IIC_SCL=0;	
		AX_Delayus(1);
    }	 
} 	 
  
  
static  u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();
    for(i=0;i<8;i++ )
	{
        IIC_SCL=0; 
        AX_Delayus(2);
		IIC_SCL=1;
        receive<<=1;
        if(READ_SDA)receive++;   
		AX_Delayus(2); 
    }					 
    if (ack)
        IIC_Ack();
    else
        IIC_NAck();
    return receive;
}


uint8_t MPU6050_I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t len, const uint8_t *data)
{
		int i;
    if (!IIC_Start())
        return 1;
    IIC_Send_Byte(dev_addr << 1 );
    if (!IIC_Wait_Ack()) {
        IIC_Stop();
        return 1;
    }
    IIC_Send_Byte(reg_addr);
    IIC_Wait_Ack();
		for (i = 0; i < len; i++) {
        IIC_Send_Byte(data[i]);
        if (!IIC_Wait_Ack()) {
            IIC_Stop();
            return 0;
        }
    }
    IIC_Stop();
    return 0;
}

uint8_t MPU6050_I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t len, uint8_t *data)
{
    if (!IIC_Start())
        return 1;
    IIC_Send_Byte(dev_addr << 1);
    if (!IIC_Wait_Ack()) {
        IIC_Stop();
        return 1;
    }
    IIC_Send_Byte(reg_addr);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte((dev_addr << 1)+1);
    IIC_Wait_Ack();
    while (len) {
        if (len == 1)
            *data = IIC_Read_Byte(0);
        else
            *data = IIC_Read_Byte(1);
        data++;
        len--;
    }
    IIC_Stop();
    return 0;
}




static void MPU6050_WriteRegister(uint8_t reg_address, uint8_t data)
{
	MPU6050_I2C_Write(MPU6050_ADDR,reg_address,1,&data);
}


static void MPU6050_ReadRegister(uint8_t reg_address, uint8_t *pdata, uint16_t len)
{
	MPU6050_I2C_Read(MPU6050_ADDR,reg_address,len,pdata);
}






