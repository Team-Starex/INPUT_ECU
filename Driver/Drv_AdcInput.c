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
static IfxVadc_Adc_Channel g_ch_accel;
static IfxVadc_Adc_Channel g_ch_brake;
static IfxVadc_Adc_Channel g_ch_steer;

static uint16 s_raw[DRV_ADC_MAX];
static uint16 s_filtered[DRV_ADC_MAX];
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static uint16 drv_adc_input_read_raw(drv_adc_channel_t ch);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void drv_adc_input_init(void)
{
    uint16 i;
    uint32 channels;
    uint32 mask;

    IfxVadc_Adc_Config        vadc_cfg;
    IfxVadc_Adc_GroupConfig   grp_cfg;
    IfxVadc_Adc_ChannelConfig ch_cfg;

    for (i = 0U; i < (uint16)DRV_ADC_MAX; i++)
    {
        s_raw[i] = 0U;
        s_filtered[i] = 0U;
    }

    /* MODULE INIT */
    IfxVadc_Adc_initModuleConfig(&vadc_cfg, &MODULE_VADC);
    IfxVadc_Adc_initModule(&g_vadc, &vadc_cfg);

    /* GROUP INIT */
    IfxVadc_Adc_initGroupConfig(&grp_cfg, &g_vadc);
    grp_cfg.groupId = DRV_ADC_GROUP_ID;
    grp_cfg.master  = DRV_ADC_GROUP_ID;

    grp_cfg.arbiter.requestSlotBackgroundScanEnabled = TRUE;
    grp_cfg.backgroundScanRequest.autoBackgroundScanEnabled = TRUE;
    grp_cfg.backgroundScanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

    IfxVadc_Adc_initGroup(&g_group, &grp_cfg);

    /* ACCEL : Group4 Channel4 -> P23.2 -> AD3 */
    IfxVadc_Adc_initChannelConfig(&ch_cfg, &g_group);
    ch_cfg.channelId = DRV_ADC_ACCEL_CH_ID;
    ch_cfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_ACCEL_CH_ID;
    ch_cfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_ch_accel, &ch_cfg);

    /* BRAKE : Group4 Channel5 -> P23.1 -> AD2 */
    IfxVadc_Adc_initChannelConfig(&ch_cfg, &g_group);
    ch_cfg.channelId = DRV_ADC_BRAKE_CH_ID;
    ch_cfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_BRAKE_CH_ID;
    ch_cfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_ch_brake, &ch_cfg);

    /* STEER : Group4 Channel6 -> P32.4 -> AD1 */
    IfxVadc_Adc_initChannelConfig(&ch_cfg, &g_group);
    ch_cfg.channelId = DRV_ADC_STEER_CH_ID;
    ch_cfg.resultRegister = (IfxVadc_ChannelResult)DRV_ADC_STEER_CH_ID;
    ch_cfg.backgroundChannel = TRUE;
    IfxVadc_Adc_initChannel(&g_ch_steer, &ch_cfg);

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

void drv_adc_input_task(void)
{
    uint16 i;
    uint32 tmp;
    uint16 raw;

    for (i = 0U; i < (uint16)DRV_ADC_MAX; i++)
    {
        raw = drv_adc_input_read_raw((drv_adc_channel_t)i);
        s_raw[i] = raw;

        tmp  = ((uint32)s_filtered[i] * DRV_ADC_FILTER_OLD);
        tmp += ((uint32)raw * DRV_ADC_FILTER_NEW);
        tmp /= DRV_ADC_FILTER_DIV;

        s_filtered[i] = (uint16)tmp;
    }
}

uint16 drv_adc_input_get_raw(drv_adc_channel_t ch)
{
    return s_raw[(uint16)ch];
}

uint16 drv_adc_input_get_filtered(drv_adc_channel_t ch)
{
    return s_filtered[(uint16)ch];
}

static uint16 drv_adc_input_read_raw(drv_adc_channel_t ch)
{
    Ifx_VADC_RES res;

    switch (ch)
    {
        case DRV_ADC_ACCEL:
            res = IfxVadc_Adc_getResult(&g_ch_accel);
            break;

        case DRV_ADC_BRAKE:
            res = IfxVadc_Adc_getResult(&g_ch_brake);
            break;

        case DRV_ADC_STEER:
            res = IfxVadc_Adc_getResult(&g_ch_steer);
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
