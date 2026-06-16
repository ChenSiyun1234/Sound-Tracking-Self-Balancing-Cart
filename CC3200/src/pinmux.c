//*****************************************************************************
// pinmux.c - route CC3200 pins to peripherals. Pin numbers from hw_config.h.
//*****************************************************************************
#include "pinmux.h"
#include "hw_config.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "pin.h"
#include "gpio.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

void PinMuxConfig(void)
{
    // Peripheral clocks
    MAP_PRCMPeripheralClkEnable(PRCM_I2CA0,  PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);
#if ENABLE_DEBUG_UART
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK);
#endif

    // Debug console UART0
#if ENABLE_DEBUG_UART
    MAP_PinTypeUART(DBG_UART_TX_PIN, DBG_UART_PIN_MODE);
    MAP_PinTypeUART(DBG_UART_RX_PIN, DBG_UART_PIN_MODE);
#endif

    // I2C0 bus (BMA222 / MPU-6050 / OLED)
    MAP_PinTypeI2C(I2C_SCL_PIN, I2C_PIN_MODE);
    MAP_PinTypeI2C(I2C_SDA_PIN, I2C_PIN_MODE);

    // Motor PWM outputs (general-purpose-timer PWM)
    MAP_PinTypeTimer(MOTOR_L_PWM_PIN, MOTOR_L_PWM_PIN_MODE);  // PIN_64 GT_PWM05
#if !MOTOR_SHARED_PWM
    MAP_PinTypeTimer(MOTOR_R_PWM_PIN, MOTOR_R_PWM_PIN_MODE);  // PIN_17 GT_PWM00
#endif

    // Motor direction + standby as GPIO (mode 0); direction set by Motor_Init()
    MAP_PinTypeGPIO(MOTOR_AIN1_PIN, PIN_MODE_0, false);
    MAP_PinTypeGPIO(MOTOR_AIN2_PIN, PIN_MODE_0, false);
    MAP_PinTypeGPIO(MOTOR_BIN1_PIN, PIN_MODE_0, false);
    MAP_PinTypeGPIO(MOTOR_BIN2_PIN, PIN_MODE_0, false);
#if MOTOR_STBY_USE_GPIO
    MAP_PinTypeGPIO(MOTOR_STBY_PIN, PIN_MODE_0, false);
#endif

    // SSD1351 OLED on GSPI (mode 7).
    //   CLK = PIN_05, MOSI = PIN_07
    //   DC = PIN_61 (GPIOA0 b6), CS = PIN_62 (GPIOA0 b7), RST = PIN_18 (GPIOA3 b4)
#if ENABLE_OLED
    MAP_PinTypeSPI(PIN_05, PIN_MODE_7);                 // GSPI CLK
    MAP_PinTypeSPI(PIN_07, PIN_MODE_7);                 // GSPI MOSI
    MAP_PinTypeGPIO(PIN_61, PIN_MODE_0, false);         // DC
    MAP_GPIODirModeSet(GPIOA0_BASE, 0x40, GPIO_DIR_MODE_OUT);
    MAP_PinTypeGPIO(PIN_62, PIN_MODE_0, false);         // CS
    MAP_GPIODirModeSet(GPIOA0_BASE, 0x80, GPIO_DIR_MODE_OUT);
    MAP_PinTypeGPIO(PIN_18, PIN_MODE_0, false);         // RST
    MAP_GPIODirModeSet(GPIOA3_BASE, 0x10, GPIO_DIR_MODE_OUT);
#endif

    // Optional wheel encoders
#if ENCODER_ENABLED
    MAP_PinTypeGPIO(ENCODER_L_A_PIN, PIN_MODE_0, false);
    MAP_PinTypeGPIO(ENCODER_R_A_PIN, PIN_MODE_0, false);
#if ENCODER_USE_QUADRATURE
    MAP_PinTypeGPIO(ENCODER_L_B_PIN, PIN_MODE_0, false);
    MAP_PinTypeGPIO(ENCODER_R_B_PIN, PIN_MODE_0, false);
#endif
#endif
}
