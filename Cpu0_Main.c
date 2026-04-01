/**********************************************************************************************************************
 * \file Cpu0_Main.c
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "Bsp.h"
#include "IfxPort.h"
#include "Drv_AdcInput.h"

IfxCpu_syncEvent cpuSyncEvent = 0;

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
#define LED_PORT                (&MODULE_P10)
#define LED_PIN                 (2U)

/* 홀센서 측정 범위 (hex 기준) */
#define HALL_RAW_MIN            (0x780U)
#define HALL_RAW_MAX            (0x850U)

/* 소프트웨어 PWM */
#define PWM_PERIOD_STEPS        (100U)
#define PWM_STEP_US             (200U)
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
volatile uint16 g_hallRaw = 0U;
volatile uint16 g_pwmDuty = 0U;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void InitLed(void);
static uint16 MapHallToDuty(uint16 raw);
static void RunLedPwm(uint16 duty);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    InitLed();
    Drv_AdcInput_Init();

    while (1)
    {
        Drv_AdcInput_Task();

        /* 엑셀 홀센서 테스트 */
        g_hallRaw = Drv_AdcInput_GetFiltered(DRV_ADC_ACCEL);

        /* 브레이크 홀센서 테스트하려면 위 줄을 아래처럼 바꾸면 됨 */
        /* g_hallRaw = Drv_AdcInput_GetFiltered(DRV_ADC_BRAKE); */

        g_pwmDuty = MapHallToDuty(g_hallRaw);

        RunLedPwm(g_pwmDuty);
    }
}

/*********************************************************************************************************************/
static void InitLed(void)
{
    IfxPort_setPinModeOutput(LED_PORT,
                             LED_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    IfxPort_setPinLow(LED_PORT, LED_PIN);
}

/*********************************************************************************************************************/
static uint16 MapHallToDuty(uint16 raw)
{
    uint32 value;

    if (raw <= HALL_RAW_MIN)
    {
        return 0U;
    }
    else if (raw >= HALL_RAW_MAX)
    {
        return PWM_PERIOD_STEPS;
    }
    else
    {
        value = ((uint32)(raw - HALL_RAW_MIN) * PWM_PERIOD_STEPS) /
                (uint32)(HALL_RAW_MAX - HALL_RAW_MIN);
        return (uint16)value;
    }
}

/*********************************************************************************************************************/
static void RunLedPwm(uint16 duty)
{
    uint16 i;

    if (duty > PWM_PERIOD_STEPS)
    {
        duty = PWM_PERIOD_STEPS;
    }

    for (i = 0U; i < PWM_PERIOD_STEPS; i++)
    {
        if (i < duty)
        {
            IfxPort_setPinHigh(LED_PORT, LED_PIN);
        }
        else
        {
            IfxPort_setPinLow(LED_PORT, LED_PIN);
        }

        waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, PWM_STEP_US));
    }
