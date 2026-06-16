/* ax_light.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


/* Includes ------------------------------------------------------------------*/
#include "ax_light.h"
#include "ax_robot.h"

uint8_t light_pixel[8][3] =
{
	{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
	{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
};

void Light_Effect1(void);
void Light_Effect2(void);
void Light_Effect3(void);
void Light_Effect4(void);
void Light_Effect5(void);
void Light_Effect6(void);



void AX_LIGHT_Show(void)
{

	if(R_Light.M == LEFFECT1)
	{
		  Light_Effect1();
	}
	
	else if(R_Light.M == LEFFECT2)
	{
		  Light_Effect2();
	}
	
	else if(R_Light.M == LEFFECT3) 
	{
		 Light_Effect3();
	}
	
	else if(R_Light.M == LEFFECT4) 
	{
		 Light_Effect4();
	}
	
	else if(R_Light.M == LEFFECT5) 
	{
		 Light_Effect5();
	}
	
	else if(R_Light.M == LEFFECT6) 
	{
		 Light_Effect6();
	}
	else 
	{
		 Light_Effect1();
	}

}		
	


void Light_Effect1(void)
{
	
	AX_RGB_SetFullColor(R_Light.R, R_Light.G, R_Light.B);
	 
}



void Light_Effect2(void)
{
	 static uint8_t cnt;
	 uint8_t ar,ag,ab;
	
	 uint8_t light;
	
	 ar = ( R_Light.R<128 ) ? 0:1;
	 ag = ( R_Light.G<128 ) ? 0:1;
	 ab = ( R_Light.B<128 ) ? 0:1;
	
	 if(cnt<41)
	 {
		 light = cnt*6;
		 AX_RGB_SetFullColor( ar*light, ag*light, ab*light );
	 }
	 else if (cnt<61)
	 {
		 light = 240;
		 AX_RGB_SetFullColor( ar*light, ag*light, ab*light );
	 }
	 else if (cnt<79)
	 {
		 light =  (80-cnt)*8;
		 AX_RGB_SetFullColor( ar*light, ag*light, ab*light );
	 }
	 else if (cnt<91)
	 {
		 light =  12;
		 AX_RGB_SetFullColor( ar*light, ag*light, ab*light );
	 }
	 else
	 {
		 cnt = 2;
	 }
	 
	 cnt++;
}

const uint8_t light3_pixel[8][3] =
{
	{0xFF,0x00,0x00},{0xFF,0xFF,0x00},{0x00,0xFF,0x00},{0xFF,0xFF,0xFF},
	{0xFF,0xFF,0xFF},{0x00,0x00,0xFF},{0x00,0xFF,0xFF},{0xFF,0x00,0xFF},
};


void Light_Effect3(void)
{

	AX_RGB_SetPixelColor1(light3_pixel);
	
}

const uint8_t light4_pixel_red[8][3] =
{
	{0x00,0x00,0x00},{0xFF,0x00,0x00},{0xFF,0x00,0x00},{0x00,0x00,0x00},
	{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
};
const uint8_t light4_pixel_blue[8][3] =
{
	{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
	{0x00,0x00,0x00},{0x00,0x00,0xFF},{0x00,0x00,0xFF},{0x00,0x00,0x00},
};

void Light_Effect4(void)
{

	 static uint8_t cnt;


	 if(cnt<3)
	 {
		 AX_RGB_SetPixelColor1(light4_pixel_red);
	 }
	 else if (cnt<6)
	 {
		 AX_RGB_SetFullColor( 0, 0, 0 );
	 }
	 else if (cnt<10)
	 {
		 AX_RGB_SetPixelColor1(light4_pixel_red);
	 }
	 else if (cnt<20)
	 {
		AX_RGB_SetFullColor( 0, 0, 0 );
	 }
	 else if(cnt<23)
	 {
		 AX_RGB_SetPixelColor1(light4_pixel_blue);
	 }
	 else if (cnt<26)
	 {
		 AX_RGB_SetFullColor( 0, 0, 0 );
	 }
	 else if (cnt<30)
	 {
		 AX_RGB_SetPixelColor1(light4_pixel_blue);
	 }
	 else if (cnt<40)
	 {
		AX_RGB_SetFullColor( 0, 0, 0 );
	 }
	 else
	 {
		 cnt = 0;
	 }
	 
	 cnt++;

}



void Light_Effect5(void)
{
	uint8_t i;
	
	light_pixel[0][0] = R_Light.R;
	light_pixel[0][1] = R_Light.G;
	light_pixel[0][2] = R_Light.B;
	
	light_pixel[7][0] = R_Light.R;
	light_pixel[7][1] = R_Light.G;
	light_pixel[7][2] = R_Light.B;
	
	for(i=1;i<7;i++)
	{
		light_pixel[i][0] = 0;
		light_pixel[i][1] = 0;
		light_pixel[i][2] = 0;
	}
	
	AX_RGB_SetPixelColor(light_pixel);
}



void Light_Effect6(void)
{
	static uint8_t i;
	
	if(i<8) 
	{
		if(i!=0)
		{
			light_pixel[i-1][0] = 0;
			light_pixel[i-1][1] = 0;
			light_pixel[i-1][2] = 0;	
		}

		light_pixel[i][0] = R_Light.R;
		light_pixel[i][1] = R_Light.G;
		light_pixel[i][2] = R_Light.B;	
		
		i++;
	}
	else if(i<16)
	{
		if(i!=8)
		{
			light_pixel[15-i+1][0] = 0;
			light_pixel[15-i+1][1] = 0;
			light_pixel[15-i+1][2] = 0;		
		}
		
		light_pixel[15-i][0] = R_Light.R;
		light_pixel[15-i][1] = R_Light.G;
		light_pixel[15-i][2] = R_Light.B;	
		
		i++;
	}
	else
	{
		i = 0;
	}
		
	AX_RGB_SetPixelColor(light_pixel);
}




