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
#define CAN_TXINPUT_NODE_ID                 IfxMultican_NodeId_0
#define CAN_TXINPUT_BAUDRATE                (500000U)
#define CAN_TXINPUT_TX_MSGOBJ_ID            (0U)

/* ShieldBuddy TC275 CAN0 핀 */
#define CAN_TXINPUT_RX_PIN                  IfxMultican_RXD0B_P20_7_IN
#define CAN_TXINPUT_TX_PIN                  IfxMultican_TXD0_P20_8_OUT

/* 외부 CAN 트랜시버 STB */
#define CAN_TXINPUT_STB_PORT                (&MODULE_P20)
#define CAN_TXINPUT_STB_PIN                 (6U)

/* busy일 때 너무 오래 막지 않기 위한 재시도 횟수 */
#define CAN_TXINPUT_SEND_RETRY_MAX          (100U)
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
static IfxMultican_Can        g_multican;
static IfxMultican_Can_Node   g_canNode;
static IfxMultican_Can_MsgObj g_canTxMsgObj;

volatile uint32 g_canSendTry  = 0U;
volatile uint32 g_canSendOk   = 0U;
volatile uint32 g_canSendBusy = 0U;
volatile uint32 g_canInitDone = 0U;
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
static void Can_TxInput_BuildMessage(const If_InputEcuData_t* data, IfxMultican_Message* message);
static void Can_TxInput_InitTransceiver(void);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void Can_TxInput_Init(void)
{
    IfxMultican_Can_Config canConfig;
    IfxMultican_Can_NodeConfig nodeConfig;
    IfxMultican_Can_MsgObjConfig msgObjConfig;

    /* 1) 외부 CAN 트랜시버를 normal mode로 전환 */
    Can_TxInput_InitTransceiver();

    /* 2) CAN module init */
    IfxMultican_Can_initModuleConfig(&canConfig, &MODULE_CAN);
    IfxMultican_Can_initModule(&g_multican, &canConfig);

    /* 3) CAN node init */
    IfxMultican_Can_Node_initConfig(&nodeConfig, &g_multican);
    nodeConfig.nodeId = CAN_TXINPUT_NODE_ID;
    nodeConfig.baudrate = CAN_TXINPUT_BAUDRATE;
    nodeConfig.rxPin = &CAN_TXINPUT_RX_PIN;
    nodeConfig.txPin = &CAN_TXINPUT_TX_PIN;
    nodeConfig.txPinMode = IfxPort_OutputMode_pushPull;
    nodeConfig.pinDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    nodeConfig.loopBackMode = FALSE;

    IfxMultican_Can_Node_init(&g_canNode, &nodeConfig);

    /* 4) TX message object init */
    IfxMultican_Can_MsgObj_initConfig(&msgObjConfig, &g_canNode);
    msgObjConfig.msgObjId = CAN_TXINPUT_TX_MSGOBJ_ID;
    msgObjConfig.messageId = CAN_TXINPUT_MSG_ID_INPUT_DATA;
    msgObjConfig.frame = IfxMultican_Frame_transmit;

    msgObjConfig.control.messageLen = IfxMultican_DataLengthCode_8;
    msgObjConfig.control.extendedFrame = FALSE;
    msgObjConfig.control.matchingId = TRUE;

    /* 일부 iLLD 버전에서는 txInterrupt/rxInterrupt 멤버가 없어서 사용하지 않음 */

    IfxMultican_Can_MsgObj_init(&g_canTxMsgObj, &msgObjConfig);

    g_canInitDone = 1U;
}

void Can_TxInput_Send(const If_InputEcuData_t* data)
{
    IfxMultican_Message message;
    IfxMultican_Status status;
    uint32 retryCnt = 0U;

    if (data == NULL_PTR)
    {
        return;
    }

    Can_TxInput_BuildMessage(data, &message);

    do
    {
        g_canSendTry++;
        status = IfxMultican_Can_MsgObj_sendMessage(&g_canTxMsgObj, &message);

        if (status == IfxMultican_Status_ok)
        {
            g_canSendOk++;
            break;
        }

        g_canSendBusy++;
        retryCnt++;
    } while (retryCnt < CAN_TXINPUT_SEND_RETRY_MAX);
}

static void Can_TxInput_BuildMessage(const If_InputEcuData_t* data, IfxMultican_Message* message)
{
    uint8 payload[8];
    uint16 buttonValue;
    uint16 brakeValue;
    uint16 accelValue;
    uint16 steerValue;
    uint32 dataLow;
    uint32 dataHigh;

    /* 요구 payload 매핑
     * byte 0~1 : button
     * byte 2~3 : brake
     * byte 4~5 : accel
     * byte 6~7 : steer
     */
    buttonValue = (data->user_ack_button == true) ? 1U : 0U;
    brakeValue  = data->brake_pedal_value;
    accelValue  = data->accel_pedal_value;
    steerValue  = data->steer_angle_deg;

    payload[0] = (uint8)(buttonValue & 0x00FFU);
    payload[1] = (uint8)((buttonValue >> 8) & 0x00FFU);

    payload[2] = (uint8)(brakeValue & 0x00FFU);
    payload[3] = (uint8)((brakeValue >> 8) & 0x00FFU);

    payload[4] = (uint8)(accelValue & 0x00FFU);
    payload[5] = (uint8)((accelValue >> 8) & 0x00FFU);

    payload[6] = (uint8)(steerValue & 0x00FFU);
    payload[7] = (uint8)((steerValue >> 8) & 0x00FFU);

    dataLow  = ((uint32)payload[0] << 0)
             | ((uint32)payload[1] << 8)
             | ((uint32)payload[2] << 16)
             | ((uint32)payload[3] << 24);

    dataHigh = ((uint32)payload[4] << 0)
             | ((uint32)payload[5] << 8)
             | ((uint32)payload[6] << 16)
             | ((uint32)payload[7] << 24);

    IfxMultican_Message_init(message,
                             CAN_TXINPUT_MSG_ID_INPUT_DATA,
                             dataLow,
                             dataHigh,
                             IfxMultican_DataLengthCode_8);
}

static void Can_TxInput_InitTransceiver(void)
{
    IfxPort_setPinModeOutput(CAN_TXINPUT_STB_PORT,
                             CAN_TXINPUT_STB_PIN,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    /* LOW = normal mode */
    IfxPort_setPinLow(CAN_TXINPUT_STB_PORT, CAN_TXINPUT_STB_PIN);
}
/*********************************************************************************************************************/
