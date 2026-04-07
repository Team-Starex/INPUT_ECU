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
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
volatile uint16 g_task_10ms_cnt   = 0U;

/* 디버그용 변수 */
volatile boolean g_led_on        = FALSE;
volatile boolean g_button_pulse  = FALSE;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void init_led(void);
static void toggle_led(void);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void core0_main(void)
{
    const if_input_ecu_data_t* input_data;

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

    init_led();
    app_input_ecu_init();

    while (1)
    {
        app_input_ecu_task_10ms();
        app_input_ecu_task_10ms_can_tx();
        g_task_10ms_cnt++;

        input_data = if_input_ecu_get_data();
        g_button_pulse = (input_data->user_ack_button == true) ? TRUE : FALSE;

        if (input_data->user_ack_button == true)
        {
            toggle_led();
        }

        waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, TASK_BASE_PERIOD_MS));
    }
}

/*********************************************************************************************************************/
static void init_led(void)
{
    IfxPort_setPinModeOutput(LED_PORT,
                             LED_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinLow(LED_PORT, LED_PIN);
    g_led_on = FALSE;
}

/*********************************************************************************************************************/
static void toggle_led(void)
{
    if (g_led_on == FALSE)
    {
        IfxPort_setPinHigh(LED_PORT, LED_PIN);
        g_led_on = TRUE;
    }
    else
    {
        IfxPort_setPinLow(LED_PORT, LED_PIN);
        g_led_on = FALSE;
    }
}
/*********************************************************************************************************************/
