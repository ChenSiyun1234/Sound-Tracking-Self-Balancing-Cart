//*****************************************************************************
// pinmux.h - device pin-mux configuration for the balance cart.
//*****************************************************************************
#ifndef __PINMUX_H__
#define __PINMUX_H__

#ifdef __cplusplus
extern "C" {
#endif

// Route CC3200 pins to peripheral functions. Call once at boot, before module init.
extern void PinMuxConfig(void);

#ifdef __cplusplus
}
#endif
#endif // __PINMUX_H__
