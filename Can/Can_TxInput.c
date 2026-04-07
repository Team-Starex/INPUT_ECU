/**********************************************************************************************************************
 * \file Can_TxInput.c
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
#include "Can_TxInput.h"
#include "IfxMultican_Can.h"
#include "IfxPort.h"
#include <stddef.h>
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
#define CAN_TX_INPUT_NODE_ID                 IfxMultican_NodeId_0
#define CAN_TX_INPUT_BAUDRATE                (500000U)
#define CAN_TX_INPUT_TX_MSGOBJ_ID            (0U)

/* ShieldBuddy TC275 CAN0 핀 */
#define CAN_TX_INPUT_RX_PIN                  IfxMultican_RXD0B_P20_7_IN
#define CAN_TX_INPUT_TX_PIN                  IfxMultican_TXD0_P20_8_OUT

/* 외부 CAN 트랜시버 STB */
#define CAN_TX_INPUT_STB_PORT                (&MODULE_P20)
#define CAN_TX_INPUT_STB_PIN                 (6U)

/* busy일 때 너무 오래 막지 않기 위한 재시도 횟수 */
#define CAN_TX_INPUT_SEND_RETRY_MAX          (100U)
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
static IfxMultican_Can        g_multi_can;
static IfxMultican_Can_Node   g_can_node;
static IfxMultican_Can_MsgObj g_can_tx_msg_obj;

volatile uint32 g_can_send_try  = 0U;
volatile uint32 g_can_send_ok   = 0U;
volatile uint32 g_can_send_busy = 0U;
volatile uint32 g_can_init_done = 0U;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void can_tx_input_build_message(const if_input_ecu_data_t* data, IfxMultican_Message* message);
static void can_tx_input_init_transceiver(void);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void can_tx_input_init(void)
{
    IfxMultican_Can_Config can_config;
    IfxMultican_Can_NodeConfig node_config;
    IfxMultican_Can_MsgObjConfig msg_obj_config;

    /* 1) 외부 CAN 트랜시버를 normal mode로 전환 */
    can_tx_input_init_transceiver();

    /* 2) CAN module init */
    IfxMultican_Can_initModuleConfig(&can_config, &MODULE_CAN);
    IfxMultican_Can_initModule(&g_multi_can, &can_config);

    /* 3) CAN node init */
    IfxMultican_Can_Node_initConfig(&node_config, &g_multi_can);
    node_config.nodeId = CAN_TX_INPUT_NODE_ID;
    node_config.baudrate = CAN_TX_INPUT_BAUDRATE;
    node_config.rxPin = &CAN_TX_INPUT_RX_PIN;
    node_config.txPin = &CAN_TX_INPUT_TX_PIN;
    node_config.txPinMode = IfxPort_OutputMode_pushPull;
    node_config.pinDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    node_config.loopBackMode = FALSE;

    IfxMultican_Can_Node_init(&g_can_node, &node_config);

    /* 4) TX message object init */
    IfxMultican_Can_MsgObj_initConfig(&msg_obj_config, &g_can_node);
    msg_obj_config.msgObjId = CAN_TX_INPUT_TX_MSGOBJ_ID;
    msg_obj_config.messageId = CAN_TX_INPUT_MSG_ID_INPUT_DATA;
    msg_obj_config.frame = IfxMultican_Frame_transmit;

    /* 8바이트 송신 */
    msg_obj_config.control.messageLen = IfxMultican_DataLengthCode_8;
    msg_obj_config.control.extendedFrame = FALSE;
    msg_obj_config.control.matchingId = TRUE;

    IfxMultican_Can_MsgObj_init(&g_can_tx_msg_obj, &msg_obj_config);

    g_can_init_done = 1U;
}

void can_tx_input_send(const if_input_ecu_data_t* data)
{
    IfxMultican_Message message;
    IfxMultican_Status status;
    uint32 retry_cnt = 0U;

    if (data == NULL_PTR)
    {
        return;
    }

    can_tx_input_build_message(data, &message);

    do
    {
        g_can_send_try++;
        status = IfxMultican_Can_MsgObj_sendMessage(&g_can_tx_msg_obj, &message);

        if (status == IfxMultican_Status_ok)
        {
            g_can_send_ok++;
            break;
        }

        g_can_send_busy++;
        retry_cnt++;
    } while (retry_cnt < CAN_TX_INPUT_SEND_RETRY_MAX);
}

static void can_tx_input_build_message(const if_input_ecu_data_t* data, IfxMultican_Message* message)
{
    uint16 button_value;
    uint16 brake_value;
    uint16 accel_value;
    uint16 steer_value;

    uint32 data_low;
    uint32 data_high;

    /* 0~1 : 버튼
     * 2~3 : 브레이크
     * 4~5 : 엑셀
     * 6~7 : 핸들
     *
     * 센서값은 0~255(uint8) 이지만,
     * 기존 바이트 위치 유지를 위해 16비트 자리로 싣고
     * 상위 바이트는 0으로 보낸다.
     *
     * little-endian 배치
     * byte0 = low byte, byte1 = high byte
     */
    button_value = (data->user_ack_button == true) ? 1U : 0U;
    brake_value  = (uint16)data->brake_pedal_value;
    accel_value  = (uint16)data->accel_pedal_value;
    steer_value  = (uint16)data->steer_angle_deg;

    data_low =
        ((uint32)(button_value & 0x00FFU)       ) |
        ((uint32)((button_value >> 8) & 0x00FFU) << 8) |
        ((uint32)(brake_value & 0x00FFU)        << 16) |
        ((uint32)((brake_value >> 8) & 0x00FFU) << 24);

    data_high =
        ((uint32)(accel_value & 0x00FFU)        ) |
        ((uint32)((accel_value >> 8) & 0x00FFU) << 8) |
        ((uint32)(steer_value & 0x00FFU)        << 16) |
        ((uint32)((steer_value >> 8) & 0x00FFU) << 24);

    IfxMultican_Message_init(message,
                             CAN_TX_INPUT_MSG_ID_INPUT_DATA,
                             data_low,
                             data_high,
                             IfxMultican_DataLengthCode_8);
}

static void can_tx_input_init_transceiver(void)
{
    IfxPort_setPinModeOutput(CAN_TX_INPUT_STB_PORT,
                             CAN_TX_INPUT_STB_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    /* LOW = normal mode */
    IfxPort_setPinLow(CAN_TX_INPUT_STB_PORT, CAN_TX_INPUT_STB_PIN);
}
/*********************************************************************************************************************/
