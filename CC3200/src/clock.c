//*****************************************************************************
// clock.c  -  free-running microsecond time-base (see clock.h)
//*****************************************************************************
#include "clock.h"
#include "hw_config.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"

//*****************************************************************************
void Clock_Init(void)
{
    MAP_PRCMPeripheralClkEnable(CLOCK_TIMER_PERIPH, PRCM_RUN_MODE_CLK);
    // 32-bit periodic timer, down-counting from 0xFFFFFFFF with auto-reload.
    MAP_TimerConfigure(CLOCK_TIMER_BASE, TIMER_CFG_PERIODIC);
    MAP_TimerLoadSet(CLOCK_TIMER_BASE, TIMER_A, 0xFFFFFFFF);
    MAP_TimerEnable(CLOCK_TIMER_BASE, TIMER_A);
}

//*****************************************************************************
unsigned long Clock_Ticks(void)
{
    return MAP_TimerValueGet(CLOCK_TIMER_BASE, TIMER_A);
}

//*****************************************************************************
// Down-counter: (start - now) in unsigned 32-bit = ticks elapsed, correct
// across one wrap.
//*****************************************************************************
unsigned long Clock_ElapsedUs(unsigned long ulStartTicks)
{
    unsigned long ulNow = Clock_Ticks();
    return (ulStartTicks - ulNow) / CLOCK_TICKS_PER_US;
}

unsigned long Clock_DeltaUs(unsigned long ulStart, unsigned long ulEnd)
{
    return (ulStart - ulEnd) / CLOCK_TICKS_PER_US;
}

//*****************************************************************************
void Clock_DelayUs(unsigned long ulUs)
{
    unsigned long ulStart = Clock_Ticks();
    while (Clock_ElapsedUs(ulStart) < ulUs) { /* spin */ }
}
