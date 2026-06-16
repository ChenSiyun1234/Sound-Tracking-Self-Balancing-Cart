//*****************************************************************************
// motor.c  -  TB6612FNG dual motor driver (see motor.h)
//*****************************************************************************
#include "motor.h"
#include "hw_config.h"

// Driverlib
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "gpio.h"
#include "rom.h"
#include "rom_map.h"

// Common GPIO helper (maps a GPIO number -> port base + pin mask)
#include "gpio_if.h"

// Cached port/pin for the digital control lines, filled in Motor_Init().
typedef struct { unsigned int uiPort; unsigned char ucPin; unsigned char ucNum; } tDigOut;
static tDigOut g_sAIN1, g_sAIN2, g_sBIN1, g_sBIN2;
#if MOTOR_STBY_USE_GPIO
static tDigOut g_sSTBY;
#endif

//*****************************************************************************
static void DigOutInit(tDigOut *p, unsigned char ucGpioNum)
{
    p->ucNum = ucGpioNum;
    GPIO_IF_GetPortNPin(ucGpioNum, &p->uiPort, &p->ucPin);
    MAP_GPIODirModeSet(p->uiPort, p->ucPin, GPIO_DIR_MODE_OUT);
}

static void DigOutSet(tDigOut *p, unsigned char ucVal)
{
    GPIO_IF_Set(p->ucNum, p->uiPort, p->ucPin, ucVal);
}

static void PwmTimerInit(unsigned long ulPeriph, unsigned long ulBase,
                         unsigned long ulTimer, unsigned long ulConfig)
{
    MAP_PRCMPeripheralClkEnable(ulPeriph, PRCM_RUN_MODE_CLK);
    MAP_TimerConfigure(ulBase, ulConfig);
    MAP_TimerPrescaleSet(ulBase, ulTimer, 0);
    MAP_TimerControlLevel(ulBase, ulTimer, 1);            // invert: level 0 -> 0% drive
    MAP_TimerLoadSet(ulBase, ulTimer, MOTOR_PWM_RELOAD);
    MAP_TimerMatchSet(ulBase, ulTimer, MOTOR_PWM_RELOAD); // start at 0% duty (stopped)
    MAP_TimerEnable(ulBase, ulTimer);
}

static void PwmSetLevel(unsigned long ulBase, unsigned long ulTimer, int iLevel)
{
    if (iLevel < 0) iLevel = 0;
    if (iLevel > MOTOR_LEVEL_MAX) iLevel = MOTOR_LEVEL_MAX;
    MAP_TimerMatchSet(ulBase, ulTimer, iLevel * MOTOR_PWM_GRANULARITY);
}

//*****************************************************************************
void Motor_Init(void)
{
    // Direction + standby GPIOs
    DigOutInit(&g_sAIN1, MOTOR_AIN1_GPIO);
    DigOutInit(&g_sAIN2, MOTOR_AIN2_GPIO);
    DigOutInit(&g_sBIN1, MOTOR_BIN1_GPIO);
    DigOutInit(&g_sBIN2, MOTOR_BIN2_GPIO);
#if MOTOR_STBY_USE_GPIO
    DigOutInit(&g_sSTBY, MOTOR_STBY_GPIO);
#endif

    // PWM channels must be on two different timer bases
    PwmTimerInit(MOTOR_L_PWM_PERIPH, MOTOR_L_PWM_BASE, MOTOR_L_PWM_TIMER, MOTOR_L_PWM_CFG);
    PwmTimerInit(MOTOR_R_PWM_PERIPH, MOTOR_R_PWM_BASE, MOTOR_R_PWM_TIMER, MOTOR_R_PWM_CFG);

    Motor_Coast();
    Motor_Enable(0);   // standby until controller is ready
}

//*****************************************************************************
void Motor_Enable(unsigned char ucEnable)
{
#if MOTOR_STBY_USE_GPIO
    DigOutSet(&g_sSTBY, ucEnable ? 1 : 0);
#else
    // STBY tied HIGH externally; disable is done by the caller via Motor_Coast().
    (void)ucEnable;
#endif
}

// ucInvert flips electrical polarity for the mirror-mounted motor.
static void DriveWheel(tDigOut *in1, tDigOut *in2,
                       unsigned long ulBase, unsigned long ulTimer,
                       int iSpeed, unsigned char ucInvert)
{
    int iMag = iSpeed < 0 ? -iSpeed : iSpeed;
    int iDir = iSpeed;                       // >0 forward, <0 back, 0 stop
    if (ucInvert) iDir = -iDir;

    if (iDir > 0)        { DigOutSet(in1, 1); DigOutSet(in2, 0); }
    else if (iDir < 0)   { DigOutSet(in1, 0); DigOutSet(in2, 1); }
    else                 { DigOutSet(in1, 0); DigOutSet(in2, 0); } // coast

    PwmSetLevel(ulBase, ulTimer, iMag);
}

void Motor_SetLeft(int iSpeed)
{
    DriveWheel(&g_sAIN1, &g_sAIN2, MOTOR_L_PWM_BASE, MOTOR_L_PWM_TIMER,
               iSpeed, MOTOR_L_INVERT);
}

void Motor_SetRight(int iSpeed)
{
    DriveWheel(&g_sBIN1, &g_sBIN2, MOTOR_R_PWM_BASE, MOTOR_R_PWM_TIMER,
               iSpeed, MOTOR_R_INVERT);
}

void Motor_SetBoth(int iLeft, int iRight)
{
    Motor_SetLeft(iLeft);
    Motor_SetRight(iRight);
}

//*****************************************************************************
void Motor_Brake(void)
{
    DigOutSet(&g_sAIN1, 1); DigOutSet(&g_sAIN2, 1);   // IN1=IN2=1 -> short brake
    DigOutSet(&g_sBIN1, 1); DigOutSet(&g_sBIN2, 1);
    PwmSetLevel(MOTOR_L_PWM_BASE, MOTOR_L_PWM_TIMER, MOTOR_LEVEL_MAX);
    PwmSetLevel(MOTOR_R_PWM_BASE, MOTOR_R_PWM_TIMER, MOTOR_LEVEL_MAX);
}

void Motor_Coast(void)
{
    DigOutSet(&g_sAIN1, 0); DigOutSet(&g_sAIN2, 0);
    DigOutSet(&g_sBIN1, 0); DigOutSet(&g_sBIN2, 0);
    PwmSetLevel(MOTOR_L_PWM_BASE, MOTOR_L_PWM_TIMER, 0);
    PwmSetLevel(MOTOR_R_PWM_BASE, MOTOR_R_PWM_TIMER, 0);
}
