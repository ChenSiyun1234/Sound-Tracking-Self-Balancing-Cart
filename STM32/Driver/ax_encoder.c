/* ax_encoder.c - quadrature encoder reading (STM32F103C8T6)
 *
 *   Encoder A = TIM2 (CH1 = PA15, CH2 = PB3)   [TIM2 partial-remap 1]
 *   Encoder B = TIM3 (CH1 = PB4,  CH2 = PB5)   [TIM3 partial-remap]
 *
 * These are all 5V-TOLERANT (FT) pins, chosen on purpose: the chassis TB6612
 * (MD220A) driver board powers the Hall encoders at 5V, so E1A/E1B/E2A/E2B come
 * out at 5V logic and can wire straight in here - no level shifter needed.
 * (The default TIM2/TIM3 pins PA0/PA1/PA6/PA7 are ADC pins and NOT 5V-tolerant,
 * which is why we remap onto the freed JTAG pins instead.)
 *
 * PA15 / PB3 / PB4 are freed for GPIO by AX_JTAG_Set(JTAG_SWD_DISABLE) in main()
 * (SWD on PA13/PA14 is kept). main() calls these Init functions AFTER that, so
 * the pins are already released. API unchanged - the vendor ax_balance.c reads
 * these counters each control tick.
 */
#include "ax_encoder.h"

void AX_ENCODER_A_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

    /* PA15 (GPIOA), PB3 (GPIOB) + AFIO for the TIM2 remap */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* TIM2 partial-remap 1: CH1 -> PA15, CH2 -> PB3 */
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;          /* TIM2_CH1 */
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;           /* TIM2_CH2 */
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_DeInit(TIM2);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = 0x0;
    TIM_TimeBaseStructure.TIM_Period        = 0xFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 6;
    TIM_ICInit(TIM2, &TIM_ICInitStructure);

    TIM2->CNT = 0;
    TIM_Cmd(TIM2, ENABLE);
}

uint16_t AX_ENCODER_A_GetCounter(void)
{
    return TIM2->CNT;
}

void AX_ENCODER_A_SetCounter(uint16_t count)
{
    TIM2->CNT = count;
}

void AX_ENCODER_B_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

    /* PB4, PB5 (GPIOB) + AFIO for the TIM3 remap */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* TIM3 partial-remap: CH1 -> PB4, CH2 -> PB5 */
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5;   /* TIM3_CH1 / CH2 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_DeInit(TIM3);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = 0x0;
    TIM_TimeBaseStructure.TIM_Period        = 0xFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 6;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    TIM3->CNT = 0;
    TIM_Cmd(TIM3, ENABLE);
}

uint16_t AX_ENCODER_B_GetCounter(void)
{
    return TIM3->CNT;
}

void AX_ENCODER_B_SetCounter(uint16_t count)
{
    TIM3->CNT = count;
}
