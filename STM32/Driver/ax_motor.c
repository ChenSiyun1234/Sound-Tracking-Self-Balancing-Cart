/* ax_motor.c - TB6612FNG dual motor driver for STM32F103C8T6
 *
 *   PWMA = PA8  (TIM1_CH1)        PWMB = PA11 (TIM1_CH4)     ~20 kHz
 *   AIN1 = PB12  AIN2 = PB13      BIN1 = PB14  BIN2 = PB15
 *   STBY = PA4   (high = enabled)  [PB5 is now Encoder B CH2]
 *
 * Speed range +/-3600 (TIM1 ARR = 3599), matching the vendor balance code's PWM
 * scale, so the ax_balance gains carry over unchanged. Direction is set by the
 * sign of the speed; flip AINx/BINx (or the sign) during bring-up if a wheel runs
 * backwards.
 *
 * API is the same the vendor ax_balance.c calls: AX_MOTOR_A_SetSpeed /
 * AX_MOTOR_B_SetSpeed (replaces the OpenCTR on-board locked-antiphase driver).
 */
#include "ax_motor.h"

#define MOTOR_PWM_MAX   3600

void AX_MOTOR_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* TIM1 + GPIOA + GPIOB are all on APB2 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM1, ENABLE);

    /* PWM outputs: PA8 (TIM1_CH1), PA11 (TIM1_CH4) - alternate-function push-pull */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* STBY: PA4 - push-pull output (off PB5, which is now Encoder B CH2) */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Direction: PB12,PB13,PB14,PB15 - push-pull output */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* TIM1 PWM time base: 72 MHz / 3600 = 20 kHz */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;
    TIM_TimeBaseStructure.TIM_Period        = MOTOR_PWM_MAX - 1;   /* 3599 */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* CH1 (PWMA) + CH4 (PWMB) in PWM1 mode */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);    /* TIM1 is an advanced timer: enable main output (MOE) */
    TIM_Cmd(TIM1, ENABLE);

    GPIO_SetBits(GPIOA, GPIO_Pin_4);     /* STBY = high -> driver enabled */

    AX_MOTOR_A_SetSpeed(0);
    AX_MOTOR_B_SetSpeed(0);
}

/* Motor A: PWMA=PA8 (CH1), AIN1=PB12, AIN2=PB13 */
void AX_MOTOR_A_SetSpeed(int16_t speed)
{
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed >= 0)
    {
        GPIO_SetBits(GPIOB,   GPIO_Pin_12);   /* AIN1 = 1 */
        GPIO_ResetBits(GPIOB, GPIO_Pin_13);   /* AIN2 = 0 */
        TIM_SetCompare1(TIM1, speed);
    }
    else
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
        GPIO_SetBits(GPIOB,   GPIO_Pin_13);
        TIM_SetCompare1(TIM1, -speed);
    }
}

/* Motor B: PWMB=PA11 (CH4), BIN1=PB14, BIN2=PB15 */
void AX_MOTOR_B_SetSpeed(int16_t speed)
{
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed >= 0)
    {
        GPIO_SetBits(GPIOB,   GPIO_Pin_14);   /* BIN1 = 1 */
        GPIO_ResetBits(GPIOB, GPIO_Pin_15);   /* BIN2 = 0 */
        TIM_SetCompare4(TIM1, speed);
    }
    else
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_14);
        GPIO_SetBits(GPIOB,   GPIO_Pin_15);
        TIM_SetCompare4(TIM1, -speed);
    }
}
