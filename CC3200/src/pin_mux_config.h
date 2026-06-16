//*****************************************************************************
// pin_mux_config.h
//
// Compatibility shim: the ported Adafruit_OLED.c (#include "pin_mux_config.h")
// only needs the PinMuxConfig() prototype. This balance project's real pinmux
// lives in pinmux.c / pinmux.h; this header just satisfies that include so the
// vendor driver compiles unmodified.
//*****************************************************************************
#ifndef __PIN_MUX_CONFIG_H__
#define __PIN_MUX_CONFIG_H__

extern void PinMuxConfig(void);

#endif // __PIN_MUX_CONFIG_H__
