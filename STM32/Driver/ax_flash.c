/* ax_flash.c - STM32 self-balancing cart firmware (adapted from XTARK OpenCTR). */


#include "ax_flash.h"
#include <stdio.h>


#define TK_FLASH_DATA_SECTOR    126   
                  
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)				  
				  
#define TK_FLASH_SIZE      256
#define TK_FLASH_WREN      1
#define TK_SECTOR_SIZE     2048
#define TK_FLASH_BASE      0x08000000
#define TK_FLASH_END       0x0803FFFF

uint16_t flash_buf[TK_SECTOR_SIZE / 2];

static void Flash_Unlock(void);
static void Flash_Lock(void);
static uint8_t Flash_GetStatus(void);
static uint8_t Flash_WaitDone(uint16_t time);
static uint8_t Flash_WriteHalfWord(uint32_t faddr, uint16_t dat);
static uint16_t Flash_ReadHalfWord(uint32_t faddr);
static void Flash_Write_NoCheck(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite);
static uint8_t Flash_ErasePage(uint32_t paddr);
static void Flash_Read(uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead);
static void Flash_Write(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite);



void AX_FLASH_Read(uint16_t addr, uint16_t *pbuff, uint8_t num)
{
	uint32_t flash_addr;	  
	
	flash_addr = TK_FLASH_BASE + TK_FLASH_DATA_SECTOR * TK_SECTOR_SIZE + addr;
	
	Flash_Read(flash_addr, pbuff, num);
}


void AX_FLASH_Write(uint16_t addr, uint16_t *pbuff, uint8_t num)
{
	uint32_t flash_addr;

	
	
	flash_addr = TK_FLASH_BASE + TK_FLASH_DATA_SECTOR * TK_SECTOR_SIZE + addr;
	
	Flash_Write(flash_addr, pbuff, num);
}


void AX_FLASH_Erase(void)
{
	Flash_ErasePage(TK_FLASH_DATA_SECTOR);
}




static void Flash_Unlock(void)
{
	FLASH->KEYR = FLASH_KEY1; 
	FLASH->KEYR = FLASH_KEY2;
}



static void Flash_Lock(void)
{
	FLASH->CR |= 1 << 7;
}


static uint8_t Flash_GetStatus(void)
{
	uint32_t res;
	res = FLASH->SR;
	
	if (res & (1 << 0))
		return 1;
	else if (res & (1 << 2))
		return 2;
	else if (res & (1 << 4))
		return 3;
	return 0;
}


static uint8_t Flash_WaitDone(uint16_t time)
{
	uint8_t res, i;
	
	do
	{
		res = Flash_GetStatus();
		if (res != 1)
			break;
		for (i = 0; i < 10; i++);
		time--;
	} while (time);
	
	
	//TIMEOUT
	if (time == 0)
		res = 0xff; 
	
	return res;
}


static uint8_t Flash_WriteHalfWord(uint32_t faddr, uint16_t dat)
{
	uint8_t res;
	
	res = Flash_WaitDone(0XFF);
	
	//OK
	if (res == 0) 
	{
		FLASH->CR |= 1 << 0;
		*(vu16 *)faddr = dat;
		res = Flash_WaitDone(0XFF);
		if (res != 1)
		{
			FLASH->CR &= ~(1 << 0);
		}
	}
	return res;
}


static uint16_t Flash_ReadHalfWord(uint32_t faddr)
{
	return *(vu16 *)faddr;
}


static void Flash_Write_NoCheck(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite)
{
	uint16_t i;
	for (i = 0; i < NumToWrite; i++)
	{
		Flash_WriteHalfWord(WriteAddr, pBuffer[i]);
		WriteAddr += 2;
	}
}


static uint8_t Flash_ErasePage(uint32_t paddr)
{
	uint8_t res = 0;
	
	res = Flash_WaitDone(0X5FFF); 
	
	
	if (res == 0)
	{
		FLASH->CR |= 1 << 1;
		FLASH->AR = paddr;
		FLASH->CR |= 1 << 6;
		res = Flash_WaitDone(0X5FFF);
		if (res != 1)
		{
			FLASH->CR &= ~(1 << 1);
		}
	}
	
	return res;	
}





static void Flash_Read(uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead)
{
	uint16_t i;
	for (i = 0; i < NumToRead; i++)
	{
		pBuffer[i] = Flash_ReadHalfWord(ReadAddr);
		ReadAddr += 2;
	}
}


static void Flash_Write(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite)
{
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;
	uint16_t i;
	uint32_t offaddr;
	
	if (WriteAddr < TK_FLASH_BASE || (WriteAddr >= (TK_FLASH_BASE + 1024 * TK_FLASH_SIZE)))
		return;								 
	
	Flash_Unlock();						 
	
	offaddr = WriteAddr - TK_FLASH_BASE;
	secpos = offaddr / TK_SECTOR_SIZE;
	secoff = (offaddr % TK_SECTOR_SIZE) / 2;
	secremain = TK_SECTOR_SIZE / 2 - secoff;
	
	if (NumToWrite <= secremain)
		secremain = NumToWrite; 
	
	while (1)
	{
		Flash_Read(secpos * TK_SECTOR_SIZE + TK_FLASH_BASE, flash_buf, TK_SECTOR_SIZE / 2); 
		
		for (i = 0; i < secremain; i++)															 
		{
			if (flash_buf[secoff + i] != 0XFFFF)
				break;
		}
		
		if (i < secremain) 
		{
			Flash_ErasePage(secpos * TK_SECTOR_SIZE + TK_FLASH_BASE); 
			
			for (i = 0; i < secremain; i++)								 
			{
				flash_buf[i + secoff] = pBuffer[i];
			}
			
			Flash_Write_NoCheck(secpos * TK_SECTOR_SIZE + TK_FLASH_BASE, flash_buf, TK_SECTOR_SIZE / 2); 
		}
		else
			Flash_Write_NoCheck(WriteAddr, pBuffer, secremain);
		
		if (NumToWrite == secremain)
			break;
		else
		{
			secpos++;
			secoff = 0;
			pBuffer += secremain;
			WriteAddr += secremain * 2;
			NumToWrite -= secremain;
			if (NumToWrite > (TK_SECTOR_SIZE / 2))
				secremain = TK_SECTOR_SIZE / 2;
			else
				secremain = NumToWrite;
		}
	};
	
	Flash_Lock(); 
}


