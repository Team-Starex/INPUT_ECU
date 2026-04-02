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
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
volatile uint16 g_task10msCnt   = 0U;
volatile uint16 g_task100msCnt  = 0U;

/* 디버그용 변수 */
volatile boolean g_ledOn        = FALSE;
volatile boolean g_buttonPulse  = FALSE;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void InitLed(void);
static void ToggleLed(void);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void core0_main(void)
{
    const If_InputEcuData_t* inputData;

    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* CAN transceiver STB(P20.6) 강제 normal mode */
    IfxPort_setPinModeOutput(&MODULE_P20,
                             6,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    IfxPort_setPinLow(&MODULE_P20, 6);

    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    InitLed();
    App_InputEcu_Init();

    while (1)
    {
        App_InputEcu_Task_10ms();
        g_task10msCnt++;

        inputData = If_InputEcu_GetData();
        g_buttonPulse = (inputData->user_ack_button == true) ? TRUE : FALSE;

        if (inputData->user_ack_button == true)
        {
            ToggleLed();
        }

        if ((g_task10msCnt % TASK_100MS_TICKS) == 0U)
        {
            App_InputEcu_Task_100ms();
            g_task100msCnt++;
        }

        waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, TASK_BASE_PERIOD_MS));
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
    g_ledOn = FALSE;
}

/*********************************************************************************************************************/
static void ToggleLed(void)
{
    if (g_ledOn == FALSE)
    {
        IfxPort_setPinHigh(LED_PORT, LED_PIN);
        g_ledOn = TRUE;
    }
    else
    {
        IfxPort_setPinLow(LED_PORT, LED_PIN);
        g_ledOn = FALSE;
    }
}
/*********************************************************************************************************************/
