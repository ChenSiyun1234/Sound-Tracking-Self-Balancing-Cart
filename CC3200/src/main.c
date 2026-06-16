//*****************************************************************************
// main.c - EEC 172 self-balancing two-wheel cart (CC3200 vehicle side)
//
// IMU (BMA222 accel + MPU-6050 gyro, Kalman-fused) -> PID -> TB6612 -> two
// 12 V gear motors, with a tilt safety cut-off and an OLED status display.
//
// State machine: INIT_HW -> CALIBRATE -> BALANCE <-> FAULT_STOP
//*****************************************************************************
#include <stdio.h>
#include <stdlib.h>

// Driverlib
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"
#include "pin.h"
#include "gpio.h"

// Project
#include "hw_config.h"
#include "pinmux.h"
#include "clock.h"
#include "i2c_if.h"
#include "uart_if.h"
#include "bma222drv.h"
#include "mpu6050.h"
#include "balance.h"
#include "motor.h"
#include "encoder.h"
#include "cart_email.h"
#if ENABLE_OLED
#include "spi.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "oled_test.h"       // colour macros (BLACK/WHITE/GREEN/...)
#endif

//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

#if ENABLE_DEBUG_UART
#define LOG(...)   Report(__VA_ARGS__)
#else
#define LOG(...)
#endif

typedef enum {
    ST_INIT_HW = 0,
    ST_CALIBRATE,
    ST_BALANCE,
    ST_FAULT_STOP,
    ST_AUTOTUNE          // relay/Ziegler-Nichols PID auto-tune
} tState;

static void BoardInit(void)
{
#ifndef USE_TIRTOS
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}

// Format a float as "<int>.<1dp>" into pcBuf (avoids printf %f).
static void Fmt1(char *pcBuf, int iBufLen, const char *pcLabel, float fVal)
{
    int iX10 = (int)(fVal * 10.0f);
    char cSign = (iX10 < 0) ? '-' : ' ';
    int iA = (iX10 < 0) ? -iX10 : iX10;
    snprintf(pcBuf, iBufLen, "%s%c%d.%d", pcLabel, cSign, iA / 10, iA % 10);
}

#if ENABLE_OLED
// SSD1351 128x128 colour OLED on GSPI: CL=PIN_05, SI=PIN_07, DC=PIN_61,
// CS=PIN_62, RST=PIN_18. Title/labels/PID drawn once (OledDrawStatic); value
// fields refreshed during balancing (OledShow). SPI is separate from the IMU I2C.
#define SPI_OLED_BIT_RATE  8000000   // 8 MHz

static void ConfigureSPIForOLED(void)     // GSPI master, mode 0
{
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_GSPI);
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           SPI_OLED_BIT_RATE, SPI_MODE_MASTER, SPI_SUB_MODE_0,
                           (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                            SPI_CS_ACTIVELOW | SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);
}

static const char *StateName(tState e)
{
    switch (e) {
        case ST_INIT_HW:    return "INIT    ";
        case ST_CALIBRATE:  return "CALIB   ";
        case ST_BALANCE:    return "BALANCE ";
        case ST_FAULT_STOP: return "FAULT   ";
        case ST_AUTOTUNE:   return "AUTOTUNE";
        default:            return "?       ";
    }
}

// Draw a string at pixel (x,y) in colour fg on black.
static void OledText(int x, int y, unsigned int uiFg, const char *pcStr)
{
    setCursor(x, y);
    setTextColor(uiFg, BLACK);
    setTextSize(1);
    Outstr((char *)pcStr);
}

static float s_fOledKp = -999.0f, s_fOledKi = -999.0f, s_fOledKd = -999.0f;

static void OledDrawPID(void)             // PID lines (only when gains change)
{
    char acA[14], acB[14], acBuf[24];
    float kp, ki, kd;
    Balance_GetGains(&kp, &ki, &kd);
    Fmt1(acA, sizeof(acA), "Kp", kp);
    Fmt1(acB, sizeof(acB), "Kd", kd);
    snprintf(acBuf, sizeof(acBuf), "%s %s    ", acA, acB);
    OledText(0, 86, GREEN, acBuf);
    Fmt1(acA, sizeof(acA), "Ki", ki);
    snprintf(acBuf, sizeof(acBuf), "%s         ", acA);
    OledText(0, 100, GREEN, acBuf);
    s_fOledKp = kp; s_fOledKi = ki; s_fOledKd = kd;
}

static void OledDrawStatic(void)          // call while the cart is held (boot)
{
    fillScreen(BLACK);
    OledText(0,  0, YELLOW, "EEC172 Balance");
    OledText(0, 16, CYAN,   "St:");
    OledText(0, 32, WHITE,  "Tilt:");
    OledText(0, 48, WHITE,  "Out:");
    OledText(0, 64, WHITE,  "Spd:");
    OledDrawPID();
}

static void OledShow(tState eState, float fTilt, int iOut, float fVel)
{
    char acTmp[14], acBuf[24];
    float kp, ki, kd;

    OledText(24, 16, CYAN, StateName(eState));

    Fmt1(acTmp, sizeof(acTmp), "", fTilt);
    snprintf(acBuf, sizeof(acBuf), "%s    ", acTmp);
    OledText(36, 32, WHITE, acBuf);

    snprintf(acBuf, sizeof(acBuf), "%d     ", iOut);
    OledText(30, 48, WHITE, acBuf);

    snprintf(acBuf, sizeof(acBuf), "%d     ", (int)(fVel * 1000.0f));
    OledText(30, 64, WHITE, acBuf);

    Balance_GetGains(&kp, &ki, &kd);                 // PID line only on change
    if (kp != s_fOledKp || ki != s_fOledKi || kd != s_fOledKd)
        OledDrawPID();
}
#endif

// Busy-wait until one control period (BALANCE_LOOP_DT_S) has elapsed since
// *pulPrev, then return the actual elapsed dt (s) and advance *pulPrev.
static float LoopPace(unsigned long *pulPrev)
{
    unsigned long ulPeriodUs = (unsigned long)(BALANCE_LOOP_DT_S * 1000000.0f);
    unsigned long ulNow;
    float fDt;
    while (Clock_ElapsedUs(*pulPrev) < ulPeriodUs) { /* spin */ }
    ulNow = Clock_Ticks();
    fDt = (float)Clock_DeltaUs(*pulPrev, ulNow) * 1e-6f;
    *pulPrev = ulNow;
    return fDt;
}

// Blocking millisecond delay (boot / self-test only, never in the control loop).
static void DelayMs(unsigned int uiMs) { while (uiMs--) Clock_DelayUs(1000); }

#if MOTOR_TEST_ON_BOOT
// Drive each wheel through a fixed sequence to verify the motor path (PWM, dir
// pins, STBY, TB6612, wiring, battery) independently of the IMU. Wheels off the
// ground before running.
static void MotorSelfTest(void)
{
    const int L = 160;                       // ~63% duty
    LOG("\n\r--- MOTOR SELF-TEST: WHEELS OFF THE GROUND ---\n\r");
    Motor_Enable(1);                         // STBY = high (driver active)

    // Each wheel driven alone; zero the other wheel first in shared-PWM mode.
    LOG("[1] LEFT wheel FORWARD (right should stay still)...\n\r");
    Motor_SetRight(0); Motor_SetLeft( L);  DelayMs(1500); Motor_Coast(); DelayMs(500);
    LOG("[2] RIGHT wheel FORWARD (left should stay still)...\n\r");
    Motor_SetLeft(0);  Motor_SetRight( L); DelayMs(1500); Motor_Coast(); DelayMs(500);
    LOG("[3] BOTH wheels FORWARD...\n\r");   Motor_SetBoth( L,  L); DelayMs(1200); Motor_Coast(); DelayMs(500);
    LOG("[4] BOTH wheels REVERSE...\n\r");   Motor_SetBoth(-L, -L); DelayMs(1200); Motor_Coast();

    Motor_Enable(0);                         // back to standby
    LOG("--- self-test done. Each wheel should move at ITS step.\n\r");
    LOG("    A wheel that never moves: in shared-PWM mode P3.9 must go to BOTH\n\r");
    LOG("    PWMA AND PWMB; also check that wheel's dir pins + motor leads.\n\r");
    LOG("    (set MOTOR_TEST_ON_BOOT 0 once motors are verified)\n\r\n\r");
}
#endif

//*****************************************************************************
// Relay (Astrom-Hagglund) + Ziegler-Nichols PID auto-tuner.
//
// Bang-bang relay (motor = +/-AT_RELAY_D toward the lean) drives a limit cycle
// around upright; from its amplitude a and period Tu, Ku = 4*D/(pi*a) gives
// conservative Z-N PD gains. Requires the feedback sign (MOTOR_*_INVERT) to be
// correct already, otherwise the cart falls and we abort.
//*****************************************************************************
#define AT_RELAY_D       120          // relay motor amplitude (0..MOTOR_LEVEL_MAX)
#define AT_WARMUP_CROSS  3            // ignore the first crossings (transient)
#define AT_MEASURE_CYC   6            // averaged oscillation cycles
#define AT_TIMEOUT_US    9000000UL    // give up if no clean oscillation in 9 s

static int   g_iAtReq = 0;            // 't' requests auto-tune; serviced in BALANCE
static int   g_iAtPrevSign, g_iAtNCross, g_iAtNMeas;
static float g_fAtPeakP, g_fAtPeakN, g_fAtSumT, g_fAtSumA;
static unsigned long g_ulAtCross, g_ulAtStart;
static float g_fAtKp, g_fAtKi, g_fAtKd;

static void Autotune_Start(void)
{
    g_iAtPrevSign = 0; g_iAtNCross = 0; g_iAtNMeas = 0;
    g_fAtPeakP = 0.0f; g_fAtPeakN = 0.0f; g_fAtSumT = 0.0f; g_fAtSumA = 0.0f;
    g_ulAtStart = Clock_Ticks();
    LOG("\n\rAUTOTUNE: relay running - hold the cart, let it wobble ~%d cycles...\n\r",
        AT_MEASURE_CYC);
}

// One auto-tune tick. Returns the relay motor command. *piDone: 0=running,
// 1=done (gains in g_fAtKp/Ki/Kd), -1=failed (timeout / no oscillation).
static int Autotune_Step(float fTilt, int *piDone)
{
    int sign = (fTilt >= 0.0f) ? 1 : -1;
    int u    = sign * AT_RELAY_D;        // drive toward the lean
    *piDone = 0;

    if (fTilt > g_fAtPeakP) g_fAtPeakP = fTilt;
    if (fTilt < g_fAtPeakN) g_fAtPeakN = fTilt;

    if (g_iAtPrevSign < 0 && sign > 0)   // positive-going zero crossing = 1 period
    {
        unsigned long ulNow = Clock_Ticks();
        if (g_iAtNCross > 0)
        {
            float fPeriod = Clock_DeltaUs(g_ulAtCross, ulNow) * 1e-6f;
            float fAmp    = (g_fAtPeakP - g_fAtPeakN) * 0.5f;
            if (g_iAtNCross >= AT_WARMUP_CROSS && fAmp > 0.1f && fPeriod > 0.02f)
            {
                g_fAtSumT += fPeriod; g_fAtSumA += fAmp; g_iAtNMeas++;
            }
        }
        g_ulAtCross = ulNow; g_iAtNCross++;
        g_fAtPeakP = 0.0f; g_fAtPeakN = 0.0f;
    }
    g_iAtPrevSign = sign;

    if (g_iAtNMeas >= AT_MEASURE_CYC)
    {
        float fTu = g_fAtSumT / (float)g_iAtNMeas;
        float fA  = g_fAtSumA / (float)g_iAtNMeas;
        float fKu = (4.0f * (float)AT_RELAY_D) / (3.14159265f * fA);
        g_fAtKp = 0.50f * fKu;           // conservative PD from the Z-N relay test
        g_fAtKd = 0.0625f * fKu * fTu;   //   = Kp * Tu/8
        g_fAtKi = 0.0f;                  //   add Ki by hand only if it drifts
        *piDone = 1;
    }
    else if (Clock_ElapsedUs(g_ulAtStart) > AT_TIMEOUT_US)
    {
        *piDone = -1;
    }
    return u;
}

#if ENABLE_DEBUG_UART
// Live PID tuning over the serial console. One keypress per tick:
//   q / a : Kp +1.0 / -1.0      w / s : Kd +0.1 / -0.1
//   e / d : Ki +0.02 / -0.02    space : print current gains
static void TuneFromUart(void)
{
    float kp, ki, kd;
    char  c;
    if (!MAP_UARTCharsAvail(CONSOLE)) return;          // non-blocking
    c = (char)MAP_UARTCharGetNonBlocking(CONSOLE);
    if (c == 't' || c == 'T') {        // request relay auto-tune (starts in BALANCE)
        g_iAtReq = 1;
        LOG("AUTOTUNE requested (will start once balancing; hold the cart!)\n\r");
        return;
    }
    if (c=='r' || c=='f' || c=='c' || c=='v') {   // velocity-loop gains
        float vkp, vki;
        Balance_GetVelGains(&vkp, &vki);
        if      (c=='r') vkp += 20.0f;   else if (c=='f') vkp -= 20.0f;
        else if (c=='c') vki += 20.0f;   else             vki -= 20.0f;
        if (vkp < 0.0f) vkp = 0.0f;
        if (vki < 0.0f) vki = 0.0f;
        Balance_SetVelGains(vkp, vki);
        LOG("VEL GAINS  VKp=%d  VKi=%d\n\r", (int)vkp, (int)vki);
        return;
    }
    Balance_GetGains(&kp, &ki, &kd);
    switch (c) {
        case 'q': kp += 1.0f;  break;   case 'a': kp -= 1.0f;  break;
        case 'w': kd += 0.1f;  break;   case 's': kd -= 0.1f;  break;
        case 'e': ki += 0.02f; break;   case 'd': ki -= 0.02f; break;
        case ' ': break;                                // print only
        default:  return;
    }
    if (kp < 0.0f) kp = 0.0f;
    if (ki < 0.0f) ki = 0.0f;
    if (kd < 0.0f) kd = 0.0f;
    Balance_SetGains(kp, ki, kd);
    LOG("GAINS  Kp=%d/10  Ki=%d/100  Kd=%d/100\n\r",
        (int)(kp*10.0f), (int)(ki*100.0f), (int)(kd*100.0f));
}
#endif

//*****************************************************************************
// Free a stuck I2C bus: a slave holding SDA low mid-byte survives a plain reset.
// Bit-bang SCL up to 9 times to clock the slave out, issue a STOP, then hand the
// pins back to the I2C peripheral. Safe on a healthy bus; run before I2C_IF_Open().
//   SCL = PIN_01 = GPIO10 (GPIOA1 bit2),  SDA = PIN_02 = GPIO11 (GPIOA1 bit3)
//*****************************************************************************
static void I2C_BusRecover(void)
{
    const unsigned long ulBase = GPIOA1_BASE;
    const unsigned char ucScl  = 0x04;     // GPIO10 within GPIOA1
    const unsigned char ucSda  = 0x08;     // GPIO11 within GPIOA1
    int i;

    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

    // Drive SCL/SDA as plain GPIO: SCL output (idle high), SDA input (read it).
    MAP_PinTypeGPIO(I2C_SCL_PIN, PIN_MODE_0, 0);   // 0 = not open-drain
    MAP_PinTypeGPIO(I2C_SDA_PIN, PIN_MODE_0, 0);
    MAP_GPIODirModeSet(ulBase, ucScl, GPIO_DIR_MODE_OUT);
    MAP_GPIODirModeSet(ulBase, ucSda, GPIO_DIR_MODE_IN);
    MAP_GPIOPinWrite(ulBase, ucScl, ucScl);

    for (i = 0; i < 9; i++)
    {
        MAP_GPIOPinWrite(ulBase, ucScl, 0);       MAP_UtilsDelay(800);   // ~60 us low
        MAP_GPIOPinWrite(ulBase, ucScl, ucScl);   MAP_UtilsDelay(800);   // ~60 us high
        if (MAP_GPIOPinRead(ulBase, ucSda) & ucSda) break;               // SDA released
    }

    // STOP: SDA low -> high while SCL is high.
    MAP_GPIODirModeSet(ulBase, ucSda, GPIO_DIR_MODE_OUT);
    MAP_GPIOPinWrite(ulBase, ucSda, 0);       MAP_UtilsDelay(800);
    MAP_GPIOPinWrite(ulBase, ucScl, ucScl);   MAP_UtilsDelay(800);
    MAP_GPIOPinWrite(ulBase, ucSda, ucSda);   MAP_UtilsDelay(800);

    // Give the pins back to the I2C peripheral.
    MAP_PinTypeI2C(I2C_SCL_PIN, I2C_PIN_MODE);
    MAP_PinTypeI2C(I2C_SDA_PIN, I2C_PIN_MODE);
}

//*****************************************************************************
//                                main
//*****************************************************************************
void main(void)
{
    tState     eState = ST_INIT_HW;
    int        iSensorsOK = 1;
    unsigned long ulPrev;
    unsigned long ulLastOled = 0;
    unsigned long ulUprightSince = 0;
    int        iWasUpright = 0;
    int        iImuFail = 0;        // consecutive IMU read failures (glitch filter)
    int        iOledOk  = 0;
#if ENABLE_WIFI_EMAIL
    int        iFallEmailSent = 0;  // email once per fall
#endif

    BoardInit();
    PinMuxConfig();
    Clock_Init();

#if ENABLE_DEBUG_UART
    InitTerm();
    ClearTerm();
#endif
    LOG("\n\r=== EEC172 Self-Balancing Cart (CC3200) ===\n\r");

    // ---- Bring up each subsystem, recording per-part results ----
    {
        // Measure the us-clock over a CPU-spin window (not Clock_DelayUs, so a
        // dead clock reports instead of hanging the boot).
        unsigned long ulT0 = Clock_Ticks();
        MAP_UtilsDelay(53333);                                  // ~2 ms @80MHz
        int iClkDt = (int)Clock_DeltaUs(ulT0, Clock_Ticks());
        int iClkOk = (iClkDt > 100);   // nonzero => timer is running

        // Recover the bus, then open at 100 kHz (STD, not 400 kHz FST): slower
        // edges ride through motor noise better.
        int iI2COk;
        I2C_BusRecover();
        iI2COk = (I2C_IF_Open(I2C_MASTER_MODE_STD) == 0);

        int iBmaOk = 0; signed char bmaA[3] = {0, 0, 0};
#if ACCEL_SOURCE_IS_BMA222
        {
            int iTry;                              // first txn after bus-recover
            for (iTry = 0; iTry < 4 && !iBmaOk && iI2COk; iTry++)   // can glitch -> retry
            {
                if (BMA222Open() == 0 && BMA222Read(&bmaA[0], &bmaA[1], &bmaA[2]) == 0)
                    iBmaOk = 1;
                else
                    MAP_UtilsDelay(80000000 / 3 / 100);   // ~10 ms
            }
        }
#endif
        int iMpuOk = 0, iMpuWho = -1;
        float mpuA[3] = {0, 0, 0}, mpuG[3] = {0, 0, 0};
        if (iI2COk) {
            iMpuOk  = (MPU6050_Init() == 0);
            iMpuWho = MPU6050_WhoAmI();
            if (iMpuOk) MPU6050_Read(mpuA, mpuG);
        }
#if ENABLE_OLED
        ConfigureSPIForOLED();     // SSD1351 on GSPI, independent of the I2C bus
        Adafruit_Init();           // reset + power-on sequence
        iOledOk = 1;               // SPI has no ACK; assume the panel is wired
#endif
        iSensorsOK = iI2COk;
#if ACCEL_SOURCE_IS_BMA222
        iSensorsOK = iSensorsOK && iBmaOk;
#endif
#if ALLOW_NO_GYRO
        if (!iMpuOk) LOG("** ACCEL-ONLY MODE: MPU6050 missing, balance will be poor **\n\r");
#else
        iSensorsOK = iSensorsOK && iMpuOk;   // gyro required for real balancing
#endif

#if DEBUG_VERBOSE
        // Power-on self-test report: each part PASS/FAIL + a live value.
        LOG("\n\r========== POWER-ON SELF-TEST ==========\n\r");
        LOG("[UART ] OK  (you are reading this)\n\r");
        LOG("[CLOCK] %s  (counter running; raw=%d)\n\r", iClkOk ? "OK" : "STUCK", iClkDt);
        LOG("[I2C  ] %s\n\r", iI2COk ? "OK (bus opened)" : "FAIL (bus did not open)");
        if (iI2COk) {                  // scan: list every 7-bit addr that ACKs
            int a; unsigned char b;
            LOG("[I2CSCAN] ACK from:");
            for (a = 0x08; a <= 0x77; a++)
                if (I2C_IF_Read((unsigned char)a, &b, 1) == 0) LOG(" 0x%x", a);
            LOG("   (want 0x18=BMA222, 0x68 or 0x69=MPU6050; OLED is on SPI now)\n\r");
        }
#if ACCEL_SOURCE_IS_BMA222
        LOG("[BMA222 @0x18] %s  accel(mg) X=%d Y=%d Z=%d\n\r",
            iBmaOk ? "OK" : "FAIL / no-ACK",
            (int)bmaA[0]*1999/127, (int)bmaA[1]*1999/127, (int)bmaA[2]*1999/127);
#else
        LOG("[BMA222] disabled (using MPU6050 accel)\n\r");
#endif
        LOG("[MPU6050 @0x68] %s  whoami=0x%x  accel(mg) X=%d Y=%d Z=%d  gyro(dps) X=%d Y=%d Z=%d\n\r",
            iMpuOk ? "OK" : "FAIL", iMpuWho,
            (int)(mpuA[0]*1000), (int)(mpuA[1]*1000), (int)(mpuA[2]*1000),
            (int)mpuG[0], (int)mpuG[1], (int)mpuG[2]);
        LOG("[OLED ] SSD1351 colour OLED on GSPI (CL=P05 SI=P07 DC=P61 CS=P62 RST=P18)\n\r");
        LOG("[MOTOR] pins PWM=PIN_64/P3.9 AIN1=PIN_63 AIN2=PIN_53 BIN1=PIN_50 BIN2=PIN_45 STBY=PIN_18/P2.2  sharedPWM=%d\n\r",
            MOTOR_SHARED_PWM);
        LOG("[CFG  ] accelSrc=%s  Kp=%d Kd=%d/10  safeTilt=%d deg  loop=%dHz\n\r",
            ACCEL_SOURCE_IS_BMA222 ? "BMA222" : "MPU6050",
            (int)BALANCE_KP, (int)(BALANCE_KD*10), (int)BALANCE_SAFE_TILT_DEG,
            (int)(1.0f/BALANCE_LOOP_DT_S));
        LOG("[OVERALL] %s\n\r", iSensorsOK ? "sensors OK -> calibrate & balance"
                                           : "SENSOR FAIL -> will sit in FAULT");
        LOG("========================================\n\r");
        LOG("Hold the board level: Z accel ~ +/-1000mg, X&Y ~ 0. Tilt it forward and\n\r");
        LOG("the 'forward' accel axis swings toward +/-1000mg; swing it and the gyro\n\r");
        LOG("axis you rotate about reads the biggest dps.\n\r\n\r");
#endif
    }

    Motor_Init();                                 // motors start stopped, STBY low
    Balance_Init();
    Encoder_Init();                               // no-op unless enabled

#if ENABLE_OLED
    // Draw fixed labels + PID once now (cart held); too slow for the balance loop.
    if (iOledOk) OledDrawStatic();
#endif

#if MOTOR_TEST_ON_BOOT
    MotorSelfTest();                              // verify motor path before balancing
#endif

#if ENABLE_WIFI_EMAIL
    // Power-up email alert (blocks ~10-20 s while connecting). Done here while the
    // cart is held, never during balancing.
    if (Email_Init() == 0)
        Email_Send("Balance cart ONLINE - powered on and starting up.");
#endif

    ulPrev = Clock_Ticks();

    //*************************************************************************
    // Main super-loop: one pass per control tick (~200 Hz), dispatched by state.
    //*************************************************************************
    for (;;)
    {
        float fDt = LoopPace(&ulPrev);
        int   iOut = 0;                  // PID motor command this tick

#if ENABLE_DEBUG_UART
        TuneFromUart();
#endif
        // Start auto-tune only from a running BALANCE state (motors armed).
        if (g_iAtReq && eState == ST_BALANCE) {
            g_iAtReq = 0; Autotune_Start(); eState = ST_AUTOTUNE;
        }

        switch (eState)
        {
        //-----------------------------------------------------------------
        case ST_INIT_HW:
            if (!iSensorsOK) { eState = ST_FAULT_STOP; break; }
            LOG("Sensors OK -> calibrating\n\r");
#if ENABLE_OLED
            if (iOledOk) {
                OledText(24, 16, CYAN,  "CALIB   ");
                OledText(0, 114, WHITE, "Hold upright+still");
            }
#endif
            eState = ST_CALIBRATE;
            break;

        //-----------------------------------------------------------------
        case ST_CALIBRATE:
            // Cart must be held upright & still here (see OLED/console).
            if (Balance_Calibrate() < 0) { iSensorsOK = 0; eState = ST_FAULT_STOP; break; }
            Balance_ResetPID();
            Motor_Enable(1);                       // arm the H-bridge
#if ENABLE_WIFI_EMAIL
            iFallEmailSent = 0;                    // re-arm the fall alert
#endif
            LOG("Calibrated. Balancing.\n\r");
#if ENABLE_OLED
            if (iOledOk) OledText(0, 114, WHITE, "                  ");    // clear hint
#endif
            ulPrev = Clock_Ticks();                // reset dt after the long cal
            eState = ST_BALANCE;
            break;

        //-----------------------------------------------------------------
        case ST_BALANCE:
            if (Balance_Sample(fDt) < 0) {         // IMU read glitched this tick
                // Coast while blind (never drive on stale data); after a few fails
                // re-open the bus; only fault if it stays dead for ~0.2 s.
                Motor_Coast();
                iImuFail++;
                if (iImuFail == 5) {
                    I2C_IF_Close();
                    I2C_BusRecover();                   // clock a stuck slave free
                    I2C_IF_Open(I2C_MASTER_MODE_STD);
                    LOG("I2C re-init (noise recovery)\n\r");
                }
                if (iImuFail > 40) {
                    Motor_Enable(0);
                    LOG("IMU lost for %d ticks -> Fault stop\n\r", iImuFail);
                    iWasUpright = 0; eState = ST_FAULT_STOP;
                }
                break;
            }
            iImuFail = 0;                          // good read -> reset glitch count
            if (Balance_IsFallen()) {              // tipped past the safe angle
                Motor_Coast(); Motor_Enable(0);
                LOG("FALLEN -> Fault stop\n\r");
                iWasUpright = 0;
                eState = ST_FAULT_STOP;
                break;
            }
            // Cascaded control (angle + velocity + turn). Targets 0,0 = stand
            // still / hold heading.
            {
                int iL, iR;
                Balance_ComputeMotors(fDt, 0, 0, &iL, &iR);
                Encoder_SetDir((iL > 0) - (iL < 0), (iR > 0) - (iR < 0));
                Motor_SetLeft(iL);
                Motor_SetRight(iR);
                iOut = iL;
            }
            break;

        //-----------------------------------------------------------------
        case ST_FAULT_STOP:
            Motor_Coast(); Motor_Enable(0);
#if ENABLE_WIFI_EMAIL
            // Email once per fall (blocking send ok here: cart is down, motors off).
            if (!iFallEmailSent) {
                Email_Send("ALERT: balance cart FELL OVER (fault stop).");
                iFallEmailSent = 1;
            }
#endif
            // Keep the filter live to auto-recover once the cart is stood back
            // up and held near upright for ~1.5 s.
            if (iSensorsOK && Balance_Sample(fDt) == 0)
            {
                float fT = Balance_GetTilt();
                int   iUpright = (fT > -6.0f && fT < 6.0f);
                iImuFail = 0;
                if (iUpright && !iWasUpright) ulUprightSince = Clock_Ticks();
                iWasUpright = iUpright;
                if (iUpright && Clock_ElapsedUs(ulUprightSince) > 1500000UL)
                {
                    Balance_ResetPID();
                    Motor_Enable(1);
#if ENABLE_WIFI_EMAIL
                    iFallEmailSent = 0;            // re-arm the fall alert
#endif
                    LOG("Recovered -> Balancing\n\r");
                    eState = ST_BALANCE;
                }
            }
            else if (((++iImuFail) % 200) == 0)   // bus still dead: unstick ~1/s
            {
                I2C_IF_Close();
                I2C_BusRecover();                  // clock a stuck slave free
                I2C_IF_Open(I2C_MASTER_MODE_STD);
                LOG("I2C re-init + bus recover (fault)\n\r");
            }
            break;

        //-----------------------------------------------------------------
        case ST_AUTOTUNE:
            if (Balance_Sample(fDt) < 0) break;          // skip glitch ticks
            if (Balance_IsFallen()) {                    // fell during the test
                Motor_Coast(); Motor_Enable(0);
                LOG("AUTOTUNE aborted: cart fell. Check the feedback SIGN "
                    "(MOTOR_*_INVERT) and try again.\n\r");
                iWasUpright = 0; eState = ST_FAULT_STOP;
                break;
            }
            {
                int iDone;
                iOut = Autotune_Step(Balance_GetTilt(), &iDone);
                Motor_SetBoth(iOut, iOut);               // apply the relay command
                if (iDone == 1) {                        // success -> apply Z-N gains
                    Balance_SetGains(g_fAtKp, g_fAtKi, g_fAtKd);
                    Balance_ResetPID();
                    LOG("AUTOTUNE DONE -> Kp=%d/10 Ki=%d/100 Kd=%d/100  (now balancing; "
                        "fine-tune with q/a w/s e/d)\n\r",
                        (int)(g_fAtKp*10.0f), (int)(g_fAtKi*100.0f), (int)(g_fAtKd*100.0f));
                    eState = ST_BALANCE;
                } else if (iDone < 0) {                  // timeout / no oscillation
                    LOG("AUTOTUNE failed: no clean oscillation in time. Try a bigger "
                        "AT_RELAY_D or tune manually.\n\r");
                    eState = ST_BALANCE;
                }
            }
            break;
        }

        //-----------------------------------------------------------------
        // Low-rate OLED / console status (~4 Hz), kept short so it doesn't
        // steal time from the IMU in the control loop.
        //-----------------------------------------------------------------
        if (Clock_ElapsedUs(ulLastOled) > 250000UL)
        {
            float fTilt = Balance_GetTilt();
            ulLastOled = Clock_Ticks();

#if DEBUG_IMU_RAW
            // Bring-up aid: print raw IMU axes to see which axis reacts to tilt
            // (accel) and to swing (gyro).
            {
                float a[3], g[3];
                if (MPU6050_Read(a, g) == 0)
                    LOG("IMU acc[mg] X=%d Y=%d Z=%d | gyro[dps] X=%d Y=%d Z=%d\n\r",
                        (int)(a[0]*1000), (int)(a[1]*1000), (int)(a[2]*1000),
                        (int)g[0], (int)g[1], (int)g[2]);
            }
#endif
#if ENABLE_OLED
            // Refresh live OLED fields at ~1 Hz (every 4th 250 ms tick).
            if (iOledOk) {
                static unsigned char s_ucOledDiv = 0;
                if ((s_ucOledDiv++ & 3) == 0)
                    OledShow(eState, fTilt, iOut, Balance_GetWheelVel());
            }
#endif
            {
                char acBuf[24];
                Fmt1(acBuf, sizeof(acBuf), "tilt=", fTilt);
#if DEBUG_VERBOSE
                // ST: 0=INIT 1=CALIBRATE 2=BALANCE 3=FAULT 4=AUTOTUNE.
                LOG("ST=%d %s out=%d vel=%dmm/s fallen=%d\n\r",
                    (int)eState, acBuf, iOut,
                    (int)(Balance_GetWheelVel() * 1000.0f), Balance_IsFallen());
#else
                if (eState == ST_BALANCE || eState == ST_FAULT_STOP)
                    LOG("%s state=%d\n\r", acBuf, (int)eState);
#endif
            }
        }
    }
}
