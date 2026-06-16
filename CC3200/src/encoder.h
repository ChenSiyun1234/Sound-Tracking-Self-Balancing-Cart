//*****************************************************************************
// encoder.h
//
// Wheel speed feedback from the two MC130 (13-line, 1:48) Hall AB encoders.
// Pins E1A/E1B (left wheel), E2A/E2B (right). Power from 3.3 V (not 5 V-tolerant).
//
// Default build counts only the A channel of each wheel for speed magnitude and
// takes sign from the last motor command (Encoder_SetDir). Set
// ENCODER_USE_QUADRATURE in hw_config.h to also read B for true quadrature.
// Compiles to no-ops when ENCODER_ENABLED is 0.
//*****************************************************************************
#ifndef __ENCODER_H__
#define __ENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif

//! Configure the encoder input pins + interrupts (no-op if disabled).
extern void Encoder_Init(void);

//! Signed cumulative tick counts since boot / last reset.
extern long Encoder_GetLeftCount(void);
extern long Encoder_GetRightCount(void);

//! Zero both counters.
extern void Encoder_Reset(void);

//! Non-quadrature counter direction: +1 forward, -1 reverse, 0 stopped.
extern void Encoder_SetDir(int iLeftSign, int iRightSign);

#ifdef __cplusplus
}
#endif
#endif // __ENCODER_H__
