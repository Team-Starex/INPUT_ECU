/**********************************************************************************************************************
 * \file Cpu0_Main.c
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "Bsp.h"
#include "IfxPort.h"
#include "App_InputEcu.h"
#include "If_InputEcu.h"

IfxCpu_syncEvent cpuSyncEvent = 0;

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
#define LED_PORT                (&MODULE_P10)
#define LED_PIN                 (2U)

#define TASK_BASE_PERIOD_MS     (10U)
#define TASK_100MS_TICKS        (10U)

/* 브레이크 raw 범위 (16진수 기준) */
#define BRAKE_RAW_MIN           (0x790U)
#define BRAKE_RAW_MAX           (0x850U)

/* 소프트웨어 PWM */
#define PWM_PERIOD_STEPS        (100U)
#define PWM_STEP_US             (100U)
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
volatile uint16 g_task10msCnt   = 0U;
volatile uint16 g_task100msCnt  = 0U;

/* 디버그용 추가 변수 */
volatile uint16 g_brakeRaw      = 0U;
volatile uint16 g_ledDuty       = 0U;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void InitLed(void);
static uint16 MapBrakeToDuty(uint16 raw);
static void RunLedPwm(uint16 duty);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void core0_main(void)
{
    const If_InputEcuData_t* inputData;

    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    InitLed();
    App_InputEcu_Init();

    while (1)
    {
        App_InputEcu_Task_10ms();
        g_task10msCnt++;

        inputData = If_InputEcu_GetData();
        g_brakeRaw = inputData->brake_raw;
        g_ledDuty = MapBrakeToDuty(g_brakeRaw);

        /* 10ms 주기 안에서 LED 밝기 제어 */
        RunLedPwm(g_ledDuty);

        if ((g_task10msCnt % TASK_100MS_TICKS) == 0U)
        {
            App_InputEcu_Task_100ms();
            g_task100msCnt++;
        }
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
static uint16 MapBrakeToDuty(uint16 raw)
{
    uint32 value;

    if (raw <= BRAKE_RAW_MIN)
    {
        return 0U;
    }
    else if (raw >= BRAKE_RAW_MAX)
    {
        return PWM_PERIOD_STEPS;
    }
    else
    {
        value = ((uint32)(raw - BRAKE_RAW_MIN) * PWM_PERIOD_STEPS) /
                (uint32)(BRAKE_RAW_MAX - BRAKE_RAW_MIN);

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
}
/*********************************************************************************************************************/
