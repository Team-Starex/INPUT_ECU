/**********************************************************************************************************************
 * \file Drv_AdcInput.c
 *********************************************************************************************************************/
#include "Drv_AdcInput.h"
#include "IfxVadc_Adc.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
#define DRV_ADC_GROUP_ID             IfxVadc_GroupId_4

/* 최종 배선
 * accel  -> P23.2 -> SAR4.4 -> AD3 -> Channel 4
 * brake  -> P23.1 -> SAR4.5 -> AD2 -> Channel 5
 * steer  -> P32.4 -> SAR4.6 -> AD1 -> Channel 6
 */
#define DRV_ADC_ACCEL_CH_ID          IfxVadc_ChannelId_4
#define DRV_ADC_BRAKE_CH_ID          IfxVadc_ChannelId_5
#define DRV_ADC_STEER_CH_ID          IfxVadc_ChannelId_6

#define DRV_ADC_FILTER_OLD           (8U)
#define DRV_ADC_FILTER_NEW           (2U)
#define DRV_ADC_FILTER_DIV           (10U)
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
static IfxVadc_Adc         g_vadc;
static IfxVadc_Adc_Group   g_group;
static IfxVadc_Adc_Channel g_chAccel;
static IfxVadc_Adc_Channel g_chBrake;
static IfxVadc_Adc_Channel g_chSteer;

static uint16 s_raw[DRV_ADC_MAX];
static uint16 s_filtered[DRV_ADC_MAX];
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static uint16 Drv_AdcInput_ReadRaw(Drv_AdcChannel_t ch);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void Drv_AdcInput_Init(void)
{
    uint16 i;
    uint32 channels;
    uint32 mask;

    IfxVadc_Adc_Config        vadcCfg;
    IfxVadc_Adc_GroupConfig   grpCfg;
    IfxVadc_Adc_ChannelConfig chCfg;

    for (i = 0U; i < (uint16)DRV_ADC_MAX; i++)
    {
        s_raw[i] = 0U;
        s_filtered[i] = 0U;
    }

    /* MODULE INIT */
    IfxVadc_Adc_initModuleConfig(&vadcCfg, &MODULE_VADC);
    IfxVadc_Adc_initModule(&g_vadc, &vadcCfg);

    /* GROUP INIT */
    IfxVadc_Adc_initGroupConfig(&grpCfg, &g_vadc);
    grpCfg.groupId = DRV_ADC_GROUP_ID;
    grpCfg.master  = DRV_ADC_GROUP_ID;

    grpCfg.arbiter.requestSlotBackgroundScanEnabled = TRUE;
    grpCfg.backgroundScanRequest.autoBackgroundScanEnabled = TRUE;
    grpCfg.backgroundScanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

    IfxVadc_Adc_initGroup(&g_group, &grpCfg);

    /* ACCEL : Group4 Channel4 -> P23.2 -> AD3 */
    IfxVadc_Adc_initChannelConfig(&chCfg, &g_group);
    chCfg.channelId = DRV_ADC_ACCEL_CH_ID;
    chCfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_ACCEL_CH_ID;
    chCfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_chAccel, &chCfg);

    /* BRAKE : Group4 Channel5 -> P23.1 -> AD2 */
    IfxVadc_Adc_initChannelConfig(&chCfg, &g_group);
    chCfg.channelId = DRV_ADC_BRAKE_CH_ID;
    chCfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_BRAKE_CH_ID;
    chCfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_chBrake, &chCfg);

    /* STEER : Group4 Channel6 -> P32.4 -> AD1 */
    IfxVadc_Adc_initChannelConfig(&chCfg, &g_group);
    chCfg.channelId = DRV_ADC_STEER_CH_ID;
    chCfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_STEER_CH_ID;
    chCfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_chSteer, &chCfg);

    /* BACKGROUND SCAN 등록 */
    channels = (1U << DRV_ADC_ACCEL_CH_ID);
    mask     = channels;
    IfxVadc_Adc_setBackgroundScan(&g_vadc, &g_group, channels, mask);

    channels = (1U << DRV_ADC_BRAKE_CH_ID);
    mask     = channels;
    IfxVadc_Adc_setBackgroundScan(&g_vadc, &g_group, channels, mask);

    channels = (1U << DRV_ADC_STEER_CH_ID);
    mask     = channels;
    IfxVadc_Adc_setBackgroundScan(&g_vadc, &g_group, channels, mask);

    /* START */
    IfxVadc_Adc_startBackgroundScan(&g_vadc);
}

void Drv_AdcInput_Task(void)
{
    uint16 i;
    uint32 tmp;
    uint16 raw;

    for (i = 0U; i < (uint16)DRV_ADC_MAX; i++)
    {
        raw = Drv_AdcInput_ReadRaw((Drv_AdcChannel_t)i);
        s_raw[i] = raw;

        tmp  = ((uint32)s_filtered[i] * DRV_ADC_FILTER_OLD);
        tmp += ((uint32)raw * DRV_ADC_FILTER_NEW);
        tmp /= DRV_ADC_FILTER_DIV;

        s_filtered[i] = (uint16)tmp;
    }
}

uint16 Drv_AdcInput_GetRaw(Drv_AdcChannel_t ch)
{
    return s_raw[(uint16)ch];
}

uint16 Drv_AdcInput_GetFiltered(Drv_AdcChannel_t ch)
{
    return s_filtered[(uint16)ch];
}

static uint16 Drv_AdcInput_ReadRaw(Drv_AdcChannel_t ch)
{
    Ifx_VADC_RES res;

    switch (ch)
    {
        case DRV_ADC_ACCEL:
            res = IfxVadc_Adc_getResult(&g_chAccel);
            break;

        case DRV_ADC_BRAKE:
            res = IfxVadc_Adc_getResult(&g_chBrake);
            break;

        case DRV_ADC_STEER:
            res = IfxVadc_Adc_getResult(&g_chSteer);
            break;

        default:
            return 0U;
    }

    if (res.B.VF == 0U)
    {
        return 0U;
    }

    return (uint16)res.B.RESULT;
}
