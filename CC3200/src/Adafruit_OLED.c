
// Standard includes
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "gpio.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"

// Common interface includes
#include "uart_if.h"
#include "pin_mux_config.h"

#include "Adafruit_SSD1351.h"

#define OLED_DC_BASE     GPIOA0_BASE
#define OLED_DC_PIN      0x40
#define OLED_CS_BASE     GPIOA0_BASE
#define OLED_CS_PIN      0x80
#define OLED_RESET_BASE  GPIOA3_BASE
#define OLED_RESET_PIN   0x10

static void OledSelect(void)
{
    GPIOPinWrite(OLED_CS_BASE, OLED_CS_PIN, 0x00);
}

static void OledDeselect(void)
{
    GPIOPinWrite(OLED_CS_BASE, OLED_CS_PIN, OLED_CS_PIN);
}

static void OledCommandMode(void)
{
    GPIOPinWrite(OLED_DC_BASE, OLED_DC_PIN, 0x00);
}

static void OledDataMode(void)
{
    GPIOPinWrite(OLED_DC_BASE, OLED_DC_PIN, OLED_DC_PIN);
}

static void OledResetLow(void)
{
    GPIOPinWrite(OLED_RESET_BASE, OLED_RESET_PIN, 0x00);
}

static void OledResetHigh(void)
{
    GPIOPinWrite(OLED_RESET_BASE, OLED_RESET_PIN, OLED_RESET_PIN);
}

static void OledWriteByte(unsigned char value)
{
    unsigned long dummy;

    MAP_SPICSEnable(GSPI_BASE);
    MAP_SPIDataPut(GSPI_BASE, value);
    MAP_SPIDataGet(GSPI_BASE, &dummy);
    MAP_SPICSDisable(GSPI_BASE);
}

void writeCommand(unsigned char c)
{
    OledCommandMode();
    OledSelect();
    OledWriteByte(c);
    OledDeselect();
}

void writeData(unsigned char c)
{
    OledDataMode();
    OledSelect();
    OledWriteByte(c);
    OledDeselect();
}

void Adafruit_Init(void)
{
    OledDeselect();
    OledDataMode();
    OledResetHigh();
    MAP_UtilsDelay(80000);

    OledResetLow();
    MAP_UtilsDelay(80000);
    OledResetHigh();
    MAP_UtilsDelay(80000);

    writeCommand(SSD1351_CMD_COMMANDLOCK);
    writeData(0x12);
    writeCommand(SSD1351_CMD_COMMANDLOCK);
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);

    writeCommand(SSD1351_CMD_CLOCKDIV);
    writeData(0xF1);

    writeCommand(SSD1351_CMD_MUXRATIO);
    writeData(127);

    writeCommand(SSD1351_CMD_SETREMAP);
    writeData(0x74);

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_SETROW);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_STARTLINE);
    if (SSD1351HEIGHT == 96)
    {
        writeData(96);
    }
    else
    {
        writeData(0);
    }

    writeCommand(SSD1351_CMD_DISPLAYOFFSET);
    writeData(0x00);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);

    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01);

    writeCommand(SSD1351_CMD_PRECHARGE);
    writeData(0x32);

    writeCommand(SSD1351_CMD_VCOMH);
    writeData(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);

    writeCommand(SSD1351_CMD_CONTRASTABC);
    writeData(0xC8);
    writeData(0x80);
    writeData(0xC8);

    writeCommand(SSD1351_CMD_CONTRASTMASTER);
    writeData(0x0F);

    writeCommand(SSD1351_CMD_SETVSL);
    writeData(0xA0);
    writeData(0xB5);
    writeData(0x55);

    writeCommand(SSD1351_CMD_PRECHARGE2);
    writeData(0x01);

    writeCommand(SSD1351_CMD_DISPLAYON);
}

void goTo(int x, int y)
{
    if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    {
        return;
    }

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData((unsigned char)x);
    writeData(SSD1351WIDTH - 1);

    writeCommand(SSD1351_CMD_SETROW);
    writeData((unsigned char)y);
    writeData(SSD1351HEIGHT - 1);

    writeCommand(SSD1351_CMD_WRITERAM);
}

unsigned int Color565(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int c;

    c = r >> 3;
    c <<= 6;
    c |= g >> 2;
    c <<= 5;
    c |= b >> 3;

    return c;
}

void fillScreen(unsigned int fillcolor)
{
    fillRect(0, 0, SSD1351WIDTH, SSD1351HEIGHT, fillcolor);
}

void fillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int fillcolor)
{
    unsigned int i;

    if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    {
        return;
    }

    if ((y + h) > SSD1351HEIGHT)
    {
        h = SSD1351HEIGHT - y;
    }

    if ((x + w) > SSD1351WIDTH)
    {
        w = SSD1351WIDTH - x;
    }

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData((unsigned char)x);
    writeData((unsigned char)(x + w - 1));
    writeCommand(SSD1351_CMD_SETROW);
    writeData((unsigned char)y);
    writeData((unsigned char)(y + h - 1));
    writeCommand(SSD1351_CMD_WRITERAM);

    for (i = 0; i < (w * h); i++)
    {
        writeData((unsigned char)(fillcolor >> 8));
        writeData((unsigned char)fillcolor);
    }
}

void drawFastVLine(int x, int y, int h, unsigned int color)
{
    unsigned int i;

    if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    {
        return;
    }

    if ((y + h) > SSD1351HEIGHT)
    {
        h = SSD1351HEIGHT - y;
    }

    if (h < 0)
    {
        return;
    }

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData((unsigned char)x);
    writeData((unsigned char)x);
    writeCommand(SSD1351_CMD_SETROW);
    writeData((unsigned char)y);
    writeData((unsigned char)(y + h - 1));
    writeCommand(SSD1351_CMD_WRITERAM);

    for (i = 0; i < (unsigned int)h; i++)
    {
        writeData((unsigned char)(color >> 8));
        writeData((unsigned char)color);
    }
}

void drawFastHLine(int x, int y, int w, unsigned int color)
{
    unsigned int i;

    if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    {
        return;
    }

    if ((x + w) > SSD1351WIDTH)
    {
        w = SSD1351WIDTH - x;
    }

    if (w < 0)
    {
        return;
    }

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData((unsigned char)x);
    writeData((unsigned char)(x + w - 1));
    writeCommand(SSD1351_CMD_SETROW);
    writeData((unsigned char)y);
    writeData((unsigned char)y);
    writeCommand(SSD1351_CMD_WRITERAM);

    for (i = 0; i < (unsigned int)w; i++)
    {
        writeData((unsigned char)(color >> 8));
        writeData((unsigned char)color);
    }
}

void drawPixel(int x, int y, unsigned int color)
{
    if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    {
        return;
    }

    if ((x < 0) || (y < 0))
    {
        return;
    }

    goTo(x, y);
    writeData((unsigned char)(color >> 8));
    writeData((unsigned char)color);
}

void invert(char v)
{
    if (v)
    {
        writeCommand(SSD1351_CMD_INVERTDISPLAY);
    }
    else
    {
        writeCommand(SSD1351_CMD_NORMALDISPLAY);
    }
}
