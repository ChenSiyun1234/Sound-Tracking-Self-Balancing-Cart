/* ax_cfg.h - persist tuned gains + midpoint in the STM32 internal flash.
 *
 * NOTE: these are IMPLEMENTED at the bottom of ax_robot.c (not a separate ax_cfg.c)
 * so they build without adding a file to the Keil project. This header is found via
 * the include path, so it needs no .uvprojx entry either.
 *
 * AX_CFG_Load() runs once at boot: if a valid config is stored it overwrites the
 * code-default gains/midpoint in ax_robot.c with the saved ones (so a power cycle
 * keeps your tuning - no recompiling). AX_CFG_Save() (BT 'W') writes the current
 * values; AX_CFG_Erase() (BT 'Z') wipes them so the next boot uses code defaults.
 */
#ifndef __AX_CFG_H
#define __AX_CFG_H

#include "stm32f10x.h"

void AX_CFG_Load(void);    /* boot: load saved gains/midpoint into the globals (if valid) */
void AX_CFG_Save(void);    /* save current gains/midpoint to flash */
void AX_CFG_Erase(void);   /* erase saved config -> next boot uses code defaults */

#endif
