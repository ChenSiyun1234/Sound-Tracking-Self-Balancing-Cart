//*****************************************************************************
// clock.h
//
// Free-running 32-bit microsecond time-base on one full-width GP timer at the
// 80 MHz system clock (1 us = 80 ticks, wraps ~every 53 s). Elapsed-time math
// uses unsigned subtraction, correct across a single wrap.
//*****************************************************************************
#ifndef __CLOCK_H__
#define __CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_TICKS_PER_US   80UL

// Configure and start the free-running timer; call once at boot.
extern void Clock_Init(void);

// Current raw value of the down-counting 32-bit timer.
extern unsigned long Clock_Ticks(void);

// Microseconds elapsed from an earlier Clock_Ticks() sample until now.
extern unsigned long Clock_ElapsedUs(unsigned long ulStartTicks);

// Microseconds between two Clock_Ticks() samples (ulStart taken first).
extern unsigned long Clock_DeltaUs(unsigned long ulStart, unsigned long ulEnd);

// Busy-wait helper.
extern void Clock_DelayUs(unsigned long ulUs);

#ifdef __cplusplus
}
#endif
#endif // __CLOCK_H__
