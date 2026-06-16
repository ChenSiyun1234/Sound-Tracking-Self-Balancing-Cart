//*****************************************************************************
// encoder.c  -  wheel encoder pulse counting (see encoder.h)
//*****************************************************************************
#include "encoder.h"
#include "hw_config.h"

#if ENCODER_ENABLED

#include "hw_types.h"
#include "hw_memmap.h"
#include "gpio.h"
#include "rom.h"
#include "rom_map.h"
#include "gpio_if.h"

static unsigned int  g_uiLAPort;  static unsigned char g_ucLAPin;
static unsigned int  g_uiRAPort;  static unsigned char g_ucRAPin;
#if ENCODER_USE_QUADRATURE
static unsigned int  g_uiLBPort;  static unsigned char g_ucLBPin;
static unsigned int  g_uiRBPort;  static unsigned char g_ucRBPin;
#endif

static volatile long g_lLeft  = 0;
static volatile long g_lRight = 0;
static volatile int  g_iLDir  = 1;
static volatile int  g_iRDir  = 1;

//*****************************************************************************
static void LeftIntHandler(void)
{
    MAP_GPIOIntClear(g_uiLAPort, g_ucLAPin);
#if ENCODER_USE_QUADRATURE
    // A rising edge: B level gives direction.
    g_lLeft += GPIO_IF_Get(ENCODER_L_B_GPIO, g_uiLBPort, g_ucLBPin) ? 1 : -1;
#else
    g_lLeft += g_iLDir;
#endif
}

static void RightIntHandler(void)
{
    MAP_GPIOIntClear(g_uiRAPort, g_ucRAPin);
#if ENCODER_USE_QUADRATURE
    g_lRight += GPIO_IF_Get(ENCODER_R_B_GPIO, g_uiRBPort, g_ucRBPin) ? 1 : -1;
#else
    g_lRight += g_iRDir;
#endif
}

//*****************************************************************************
void Encoder_Init(void)
{
    GPIO_IF_GetPortNPin(ENCODER_L_A_GPIO, &g_uiLAPort, &g_ucLAPin);
    GPIO_IF_GetPortNPin(ENCODER_R_A_GPIO, &g_uiRAPort, &g_ucRAPin);
    MAP_GPIODirModeSet(g_uiLAPort, g_ucLAPin, GPIO_DIR_MODE_IN);
    MAP_GPIODirModeSet(g_uiRAPort, g_ucRAPin, GPIO_DIR_MODE_IN);

#if ENCODER_USE_QUADRATURE
    GPIO_IF_GetPortNPin(ENCODER_L_B_GPIO, &g_uiLBPort, &g_ucLBPin);
    GPIO_IF_GetPortNPin(ENCODER_R_B_GPIO, &g_uiRBPort, &g_ucRBPin);
    MAP_GPIODirModeSet(g_uiLBPort, g_ucLBPin, GPIO_DIR_MODE_IN);
    MAP_GPIODirModeSet(g_uiRBPort, g_ucRBPin, GPIO_DIR_MODE_IN);
#endif

    // Count on the A-channel rising edge of each wheel.
    GPIO_IF_ConfigureNIntEnable(g_uiLAPort, g_ucLAPin,
                                GPIO_RISING_EDGE, LeftIntHandler);
    GPIO_IF_ConfigureNIntEnable(g_uiRAPort, g_ucRAPin,
                                GPIO_RISING_EDGE, RightIntHandler);
}

long Encoder_GetLeftCount(void)  { return g_lLeft;  }
long Encoder_GetRightCount(void) { return g_lRight; }
void Encoder_Reset(void)         { g_lLeft = 0; g_lRight = 0; }

void Encoder_SetDir(int iLeftSign, int iRightSign)
{
    g_iLDir = (iLeftSign  < 0) ? -1 : (iLeftSign  > 0 ? 1 : 0);
    g_iRDir = (iRightSign < 0) ? -1 : (iRightSign > 0 ? 1 : 0);
}

#else  // ENCODER_ENABLED == 0 : compile to no-ops

void Encoder_Init(void) {}
long Encoder_GetLeftCount(void)  { return 0; }
long Encoder_GetRightCount(void) { return 0; }
void Encoder_Reset(void) {}
void Encoder_SetDir(int iLeftSign, int iRightSign) { (void)iLeftSign; (void)iRightSign; }

#endif
