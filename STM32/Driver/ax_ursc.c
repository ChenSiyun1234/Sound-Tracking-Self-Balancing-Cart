/**
 * ax_ursc.c - HC-SR04 ultrasonic ranging (STM32F103C8T6)
 *
 *   TRIG = PB0   (push-pull output, ~15us trigger pulse)
 *   ECHO = PA12  (digital input; 5V-tolerant FT pin, so the HC-SR04 5V echo wires
 *                 straight in. NOTE: PA12 is the USB D+ line and most Blue Pills
 *                 carry a ~1.5k pull-up to 3.3V on it - harmless here, the HC-SR04
 *                 push-pull echo drives both levels fine. ECHO moved off PB3
 *                 because PB3 is now Encoder A CH2.)
 *
 * The echo high-time is measured with the Cortex-M3 DWT cycle counter (the C8T6 /
 * medium-density part has no spare TIM5/6/7). Bounded-blocking poll, called from
 * the low-priority ultrasonic task. AX_URSC_GetDistance() triggers a ping, times
 * the echo and returns the distance in centimetres (0 = no echo / out of range).
 */
#include "ax_ursc.h"
#include "ax_delay.h"

/* This project's older CMSIS core_cm3.h defines CoreDebug but NOT the DWT cycle
   counter, so declare the two DWT registers we need by hand. DWT base = 0xE0001000:
   CTRL @ +0x00, CYCCNT @ +0x04. (Guarded so a newer CMSIS that defines DWT wins.) */
#ifndef DWT_CTRL_CYCCNTENA_Msk
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#endif
#ifndef DWT
typedef struct { volatile uint32_t CTRL; volatile uint32_t CYCCNT; } AX_DWT_Type;
#define DWT ((AX_DWT_Type *)0xE0001000UL)
#endif

#define URSC_TRIG_PORT   GPIOB
#define URSC_TRIG_PIN    GPIO_Pin_0       /* PB0 */
#define URSC_ECHO_PORT   GPIOA
#define URSC_ECHO_PIN    GPIO_Pin_12      /* PA12 */
#define URSC_TIMEOUT_US  30000u           /* ~5 m round-trip ceiling */

/* Cortex-M3 DWT cycle counter as a free-running microsecond time base */
static void ursc_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void AX_URSC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* TRIG = PB0, push-pull output */
    GPIO_InitStructure.GPIO_Pin   = URSC_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(URSC_TRIG_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(URSC_TRIG_PORT, URSC_TRIG_PIN);

    /* ECHO = PA12, floating input (HC-SR04 push-pull drives both levels) */
    GPIO_InitStructure.GPIO_Pin  = URSC_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(URSC_ECHO_PORT, &GPIO_InitStructure);

    ursc_dwt_init();
}

void AX_URSC_StartUp(void)
{
    GPIO_SetBits(URSC_TRIG_PORT, URSC_TRIG_PIN);
    AX_Delayus(15);
    GPIO_ResetBits(URSC_TRIG_PORT, URSC_TRIG_PIN);
}

uint32_t AX_URSC_GetDistance(void)
{
    uint32_t guard;
    uint32_t cyc_per_us  = SystemCoreClock / 1000000u;          /* 72 */
    uint32_t timeout_cyc = URSC_TIMEOUT_US * cyc_per_us;
    uint32_t c0, high_us;

    AX_URSC_StartUp();

    /* wait for echo to go high (start of pulse) */
    guard = 0;
    while (GPIO_ReadInputDataBit(URSC_ECHO_PORT, URSC_ECHO_PIN) == Bit_RESET)
    {
        AX_Delayus(1);
        if (++guard > URSC_TIMEOUT_US)
            return 0;                       /* no echo */
    }

    /* time the high pulse with the DWT cycle counter (wrap-safe unsigned diff) */
    c0 = DWT->CYCCNT;
    while (GPIO_ReadInputDataBit(URSC_ECHO_PORT, URSC_ECHO_PIN) != Bit_RESET)
    {
        if ((DWT->CYCCNT - c0) > timeout_cyc)
            return 0;                       /* echo stuck high / out of range */
    }
    high_us = (DWT->CYCCNT - c0) / cyc_per_us;

    /* distance = high_time * speed_of_sound / 2 ; ~340 m/s -> cm */
    return high_us * 17 / 1000;
}
