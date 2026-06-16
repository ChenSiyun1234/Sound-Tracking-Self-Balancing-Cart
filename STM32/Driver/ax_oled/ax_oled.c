/* ax_oled.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */

 
#include "ax_oled.h"
#include "ax_oled_ascii.h"

#define OLED_DC_L()    GPIO_ResetBits(GPIOA,GPIO_Pin_4)
#define OLED_DC_H()    GPIO_SetBits(GPIOA,GPIO_Pin_4)

#define OLED_RST_L()   GPIO_ResetBits(GPIOA,GPIO_Pin_6)
#define OLED_RST_H()   GPIO_SetBits(GPIOA,GPIO_Pin_6)

static uint8_t SPI1_Send_Byte(uint8_t dat);
static void OLED_WriteCmd(u8 data);
static void OLED_WriteData(u8 data);
static void OLED_Set_Pos(unsigned char x, unsigned char y); 


void AX_OLED_Init(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	//DC 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//RST
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);
	
	SPI1_Send_Byte(0xff);
	
    OLED_RST_H();
	AX_Delayms(100);
	OLED_RST_L();
	AX_Delayms(200);
	OLED_RST_H(); 
					  
	OLED_WriteCmd(0xAE);//--turn off oled panel
	OLED_WriteCmd(0x00);//---set low column address
	OLED_WriteCmd(0x10);//---set high column address
	OLED_WriteCmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WriteCmd(0x81);//--set contrast control register
	OLED_WriteCmd(0xEF); // Set SEG Output Current Brightness
	OLED_WriteCmd(0xA1);
	OLED_WriteCmd(0xC8);
	OLED_WriteCmd(0xA6);//--set normal display
	OLED_WriteCmd(0xA8);//--set multiplex ratio(1 to 64)
	OLED_WriteCmd(0x3f);//--1/64 duty
	OLED_WriteCmd(0xD3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WriteCmd(0x00);//-not offset
	OLED_WriteCmd(0xd5);//--set display clock divide ratio/oscillator frequency
	OLED_WriteCmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WriteCmd(0xD9);//--set pre-charge period
	OLED_WriteCmd(0xF1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WriteCmd(0xDA);//--set com pins hardware configuration
	OLED_WriteCmd(0x12);
	OLED_WriteCmd(0xDB);//--set vcomh
	OLED_WriteCmd(0x40);//Set VCOM Deselect Level
	OLED_WriteCmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WriteCmd(0x02);//
	OLED_WriteCmd(0x8D);//--set Charge Pump enable/disable
	OLED_WriteCmd(0x14);//--set(0x10) disable
	OLED_WriteCmd(0xA4);// Disable Entire Display On (0xa4/0xa5)
	OLED_WriteCmd(0xA6);// Disable Inverse Display On (0xa6/a7) 
	OLED_WriteCmd(0xAF);//--turn on oled panel
	
	AX_OLED_ClearScreen();
}


void AX_OLED_ClearScreen(void)
{   	
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WriteCmd(0xb0+i);
		OLED_WriteCmd(0x00);
		OLED_WriteCmd(0x10);
		for(n=0;n<128;n++)
			OLED_WriteData(0); 
	}
}



void AX_OLED_DispChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t mode)
{   	
	uint8_t c=0,i=0;	

	c = ch - ' ';
	
	if((x<123) && (y<8))
	{
		if(mode != 0)
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<6;i++)
				OLED_WriteData(~ASCII6X8[c*6+i]);		
		}
		else
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<6;i++)
				OLED_WriteData(ASCII6X8[c*6+i]);
		}
	}
}



void AX_OLED_DispStr(uint8_t x, uint8_t y, uint8_t *ch, uint8_t mode)
{   	
	uint8_t j=0;
	
	if((x<123) && (y<8))
	{
		while (ch[j]!='\0')
		{
			AX_OLED_DispChar(x, y, ch[j], mode);
				x+=6;
			
			if(x>122)
			{
				x=0;
				y+=1;
			}
			j++;
		}				
	}
}

void AX_OLED_DispNum(uint8_t x, uint8_t y, uint8_t num, uint8_t mode)
{
	if(num < 10) 
	{	
		AX_OLED_DispChar(x, y, ('0'+num), mode);
	}
}
 

void AX_OLED_DispValue(uint8_t x, uint8_t y, int32_t value, uint8_t inte, uint8_t deci, uint8_t mode)
{
	
	u8 dis_number[10];
	u8 i;
	u32 temp;
	
	if((inte>0) && ((inte+deci)<10))
	{
		if(value < 0)
			temp = (0-value);
		else
			temp = value;
		
		dis_number[1] = temp%10; 
		dis_number[2] = (temp%100)/10;
		dis_number[3] = (temp%1000)/100;
		dis_number[4] = (temp%10000)/1000;
		dis_number[5] = (temp%100000)/10000;
		dis_number[6] = (temp%1000000)/100000;
		dis_number[7] = (temp%10000000)/1000000;
		dis_number[8] = (temp%100000000)/10000000;
		dis_number[9] = (temp%1000000000)/100000000;		
		  
		if(value<0)
		{
		   AX_OLED_DispChar(x, y, '-', 0);
		   
		   for(i=0; i<(inte-1); i++ )
		   {
			   AX_OLED_DispNum(x+6+(i*6), y, dis_number[inte+deci-i-1],mode);
		   }
		   if(deci != 0 )
		   {
			   AX_OLED_DispChar(x+(inte*6), y, '.', mode);
			   
			   for(i=0; i<deci;i++ )
			   {
				   AX_OLED_DispNum(x+(inte*6)+6+(i*6), y, dis_number[deci-i],mode);
			   }
		   } 
		}
		else
		{	   
		   for(i=0; i<inte; i++ )
		   {
			   AX_OLED_DispNum(x+(i*6), y,dis_number[inte+deci-i],mode);
		   }
		   if(deci != 0 )
		   {
			   AX_OLED_DispChar(x+(inte*6), y, '.', mode);
			   
			   for(i=0; i<deci;i++ )
			   {
				   AX_OLED_DispNum(x+(inte*6)+6+(i*6), y, dis_number[deci-i], mode);
			   }
		   } 	   
		}
	}
	else
	{
		//AX_OLED_DisplayChar(x, y, 'X', 0);
	}	
}


void AX_OLED_Disp16Char(uint8_t x, uint8_t y, uint8_t ch, uint8_t mode)
{   	
	uint8_t c=0,i=0;	

	c = ch - ' ';
	
	if((x<121) && (y<7))
	{
		if(mode != 0)
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<8;i++)
				OLED_WriteData(~ASCII8X16[c*16+i]);
			OLED_Set_Pos(x,y+1);
			for(i=0;i<8;i++)
				OLED_WriteData(~ASCII8X16[c*16+i+8]);			
		}
		else
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<8;i++)
				OLED_WriteData(ASCII8X16[c*16+i]);
			OLED_Set_Pos(x,y+1);
			for(i=0;i<8;i++)
				OLED_WriteData(ASCII8X16[c*16+i+8]);			
		}
		
	}
}


void AX_OLED_Disp16Str(uint8_t x, uint8_t y, uint8_t *ch, uint8_t mode)
{   	
	uint8_t j=0;
	
	if((x<121) && (y<7))
	{
		while (ch[j]!='\0')
		{
			AX_OLED_Disp16Char(x, y, ch[j], mode);
				x+=8;
			
			if(x>120)
			{
				x=0;
				y+=2;
			}
			j++;
		}				
	}
}



void AX_OLED_Disp16Num(uint8_t x, uint8_t y, uint8_t num, uint8_t mode)
{
	if(num < 10) 
	{	
		AX_OLED_Disp16Char(x, y, ('0'+num), mode);
	}
}
 

void AX_OLED_Disp16Value(uint8_t x, uint8_t y, int32_t value, uint8_t inte, uint8_t deci, uint8_t mode)
{
	
	uint8_t dis_number[10];
	uint8_t i;
	u32 temp;
	
	if((inte>0) && ((inte+deci)<10))
	{
		if(value < 0)
			temp = (0-value);
		else
			temp = value;
		
		dis_number[1] = temp%10; 
		dis_number[2] = (temp%100)/10;
		dis_number[3] = (temp%1000)/100;
		dis_number[4] = (temp%10000)/1000;
		dis_number[5] = (temp%100000)/10000;
		dis_number[6] = (temp%1000000)/100000;
		dis_number[7] = (temp%10000000)/1000000;
		dis_number[8] = (temp%100000000)/10000000;
		dis_number[9] = (temp%1000000000)/100000000;		
		  
		if(value<0)
		{
		   AX_OLED_Disp16Char(x, y, '-', 0);
		   
		   for(i=0; i<(inte-1); i++ )
		   {
			   AX_OLED_Disp16Num(x+8+(i*8), y, dis_number[inte+deci-i-1],0);
		   }
		   if(deci != 0 )
		   {
			   AX_OLED_Disp16Char(x+(inte*8), y, '.', 0);
			   
			   for(i=0; i<deci;i++ )
			   {
				   AX_OLED_Disp16Num(x+(inte*8)+8+(i*8), y, dis_number[deci-i],0);
			   }
		   } 
		}
		else
		{	   
		   for(i=0; i<inte; i++ )
		   {
			   AX_OLED_Disp16Num(x+(i*8), y,dis_number[inte+deci-i],0);
		   }
		   if(deci != 0 )
		   {
			   AX_OLED_Disp16Char(x+(inte*8), y, '.', 0);
			   
			   for(i=0; i<deci;i++ )
			   {
				   AX_OLED_Disp16Num(x+(inte*8)+8+(i*8), y, dis_number[deci-i], 0);
			   }
		   } 	   
		}
	}
	else
	{
		//AX_OLED_DisplayChar(x, y, 'X', 0);
	}	
	
}



void AX_OLED_DispChinese(uint8_t x, uint8_t y, const uint8_t *pbuf, uint8_t mode)
{      			    
	uint8_t t;

	if((x<113) && (y<7))
	{	
		if(mode)
		{
			OLED_Set_Pos(x,y);
			for(t=0;t<16;t++)
			{
				OLED_WriteData(~*pbuf++);
			}
			OLED_Set_Pos(x,y+1);	
			for(t=0;t<16;t++)
			{
				OLED_WriteData(~*pbuf++);
			}			
		}
		else
		{
			OLED_Set_Pos(x,y);
			for(t=0;t<16;t++)
			{
				OLED_WriteData(*pbuf++);
			}
			OLED_Set_Pos(x,y+1);	
			for(t=0;t<16;t++)
			{
				OLED_WriteData(*pbuf++);
			}	
		}
	}		
}



void AX_OLED_DispPicture(uint8_t x, uint8_t y, uint8_t xsize, uint8_t ysize, const uint8_t *pbuf, uint8_t mode)   
{
	uint8_t tx,ty;
	
	if(x>127 || y>7)
		return;
	if((x+xsize)>128 || (y+ysize)>8)
		return;

	if(mode)
	{
		for(ty=y; ty<ysize; ty++)
		{
			OLED_Set_Pos(x,ty);
			for(tx=x; tx<xsize; tx++)
			{
				OLED_WriteData(~*pbuf++);			
			}
		}		
		
	}
	else
	{
		for(ty=y; ty<ysize; ty++)
		{
			OLED_Set_Pos(x,ty);
			for(tx=x; tx<xsize; tx++)
			{
				OLED_WriteData(*pbuf++);			
			}
		}		
	}
	
}




static uint8_t SPI1_Send_Byte(uint8_t dat)
{
  /* Loop while DR register in not emplty */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

  /* Send byte through the SPI peripheral */
  SPI_I2S_SendData(SPI1, dat);

  /* Wait to receive a byte */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI1);
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
static void OLED_WriteCmd(u8 data)
{	  
	OLED_DC_L();
	                  
    SPI1_Send_Byte(data);

	OLED_DC_H(); 
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
static void OLED_WriteData(u8 data)
{	               
    SPI1_Send_Byte(data);
}


static void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 
	OLED_WriteCmd(0xb0+y);
	OLED_WriteCmd(((x&0xf0)>>4)|0x10);
	OLED_WriteCmd((x&0x0f)|0x01); 
} 

