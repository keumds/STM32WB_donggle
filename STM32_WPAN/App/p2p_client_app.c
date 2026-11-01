/* USER CODE BEGIN Header */
/**
  ******************************************************************************
 * @file    p2p_server_app.c
 * @author  MCD Application Team
 * @brief   peer to peer Server Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "p2p_client_app.h"

#include "stm32_seq.h"
#include "app_ble.h"

#include "p1_integration.h"
#include "p1_bridge.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */


/* P1 128-bit UUIDs, serialized in BLE little-endian order. */
static const uint8_t P1_UUID_SERVICE[16] = {
  0x8F,0xE5,0xB3,0xD5,0x2E,0x7F,0x4A,0x98,
  0x2A,0x48,0x7A,0xCC,0x40,0xFE,0x00,0x00
};
static const uint8_t P1_UUID_FE41[16] = {
  0x19,0xED,0x82,0xAE,0xED,0x21,0x4C,0x9D,
  0x41,0x45,0x22,0x8E,0x41,0xFE,0x00,0x00
};
static const uint8_t P1_UUID_FE42[16] = {
  0x19,0xED,0x82,0xAE,0xED,0x21,0x4C,0x9D,
  0x41,0x45,0x22,0x8E,0x42,0xFE,0x00,0x00
};
static const uint8_t P1_UUID_FE43[16] = {
  0x19,0xED,0x82,0xAE,0xED,0x21,0x4C,0x9D,
  0x41,0x45,0x22,0x8E,0x43,0xFE,0x00,0x00
};
static int P1_UuidMatch(const uint8_t *a, const uint8_t *b)
{
  unsigned i;
  for (i=0; i<16; ++i) if (a[i]!=b[i]) return 0;
  return 1;
}

/* Private typedef -----------------------------------------------------------*/

typedef enum
{
  P2P_START_TIMER_EVT,
  P2P_STOP_TIMER_EVT,
  P2P_NOTIFICATION_INFO_RECEIVED_EVT,
} P2P_Client_Opcode_Notification_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}P2P_Client_Data_t;

typedef struct
{
  P2P_Client_Opcode_Notification_evt_t  P2P_Client_Evt_Opcode;
  P2P_Client_Data_t DataTransfered;
}P2P_Client_App_Notification_evt_t;

typedef struct
{
  /**
   * state of the P2P Client
   * state machine
   */
  APP_BLE_ConnStatus_t state;

  /**
   * connection handle
   */
  uint16_t connHandle;

  /**
   * handle of the P2P service
   */
  uint16_t P2PServiceHandle;

  /**
   * end handle of the P2P service
   */
  uint16_t P2PServiceEndHandle;

  /**
   * handle of the Tx characteristic - Write To Server
   *
   */
  uint16_t P2PWriteToServerCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Tx characteristic
   */
  uint16_t P2PWriteToServerDescHandle;

  /**
   * handle of the Rx characteristic - Notification From Server
   *
   */
  uint16_t P2PNotificationCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Rx characteristic
   */
  uint16_t P2PNotificationDescHandle;
  uint16_t FE42CharHandle;
  uint16_t FE43CharHandle;
  uint16_t FE43DescHandle;
  uint8_t p1_char_discovery_pending;
  uint8_t p1_char_discovery_failed;
  uint8_t p1_mtu_exchange_pending;
  uint8_t p1_mtu_exchange_done;
  uint16_t p1_negotiated_mtu;

}P2P_ClientContext_t;

/* USER CODE BEGIN PTD */
typedef struct{
  uint8_t                                     Device_Led_Selection;
  uint8_t                                     Led1;
}P2P_LedCharValue_t;

typedef struct{
  uint8_t                                     Device_Button_Selection;
  uint8_t                                     Button1;
}P2P_ButtonCharValue_t;

typedef struct
{

  uint8_t       Notification_Status; /* used to check if P2P Server is enabled to Notify */

  P2P_LedCharValue_t         LedControl;
  P2P_ButtonCharValue_t      ButtonStatus;

  uint16_t ConnectionHandle; 


} P2P_Client_App_Context_t;

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
#define UNPACK_2_BYTE_PARAMETER(ptr)  \
        (uint16_t)((uint16_t)(*((uint8_t *)ptr))) |   \
        (uint16_t)((((uint16_t)(*((uint8_t *)ptr + 1))) << 8))
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static P2P_ClientContext_t aP2PClientContext[BLE_CFG_CLT_MAX_NBR_CB];

/**
 * END of Section BLE_APP_CONTEXT
 */
/* USER CODE BEGIN PV */
static P2P_Client_App_Context_t P2P_Client_App_Context;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification);
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event);
/* USER CODE BEGIN PFP */
static tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload);
static void Button_Trigger_Received( void );
static void Update_Service( void );
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void P2PC_APP_Init(void)
{
  uint8_t index =0;
/* USER CODE BEGIN P2PC_APP_Init_1 */
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SEARCH_SERVICE_ID, UTIL_SEQ_RFU, Update_Service );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW1_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button_Trigger_Received );

  /**
   * Initialize LedButton Service
   */
  P2P_Client_App_Context.Notification_Status=0;
  P2P_Client_App_Context.ConnectionHandle =  0x00;
      P1_Disconnected();

  P2P_Client_App_Context.LedControl.Device_Led_Selection=0x00;/* device Led */
  P2P_Client_App_Context.LedControl.Led1=0x00; /* led OFF */
  P2P_Client_App_Context.ButtonStatus.Device_Button_Selection=0x01;/* Device1 */
  P2P_Client_App_Context.ButtonStatus.Button1=0x00;
/* USER CODE END P2PC_APP_Init_1 */
  for(index = 0; index < BLE_CFG_CLT_MAX_NBR_CB; index++)
  {
    aP2PClientContext[index].FE42CharHandle = 0;
    aP2PClientContext[index].FE43CharHandle = 0;
    aP2PClientContext[index].FE43DescHandle = 0;
    aP2PClientContext[index].p1_char_discovery_pending = 0;
    aP2PClientContext[index].p1_char_discovery_failed = 0;
    aP2PClientContext[index].state= APP_BLE_IDLE;
  }

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterCltHandler(Event_Handler);

#if(CFG_DEBUG_APP_TRACE != 0)
  APP_DBG_MSG("-- P2P CLIENT INITIALIZED \n");
#endif

/* USER CODE BEGIN P2PC_APP_Init_2 */

/* USER CODE END P2PC_APP_Init_2 */
  return;
}

void P2PC_APP_Notification(P2PC_APP_ConnHandle_Not_evt_t *pNotification)
{
/* USER CODE BEGIN P2PC_APP_Notification_1 */

/* USER CODE END P2PC_APP_Notification_1 */
  switch(pNotification->P2P_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Evt_Opcode */

/* USER CODE END P2P_Evt_Opcode */

  case PEER_CONN_HANDLE_EVT :
/* USER CODE BEGIN PEER_CONN_HANDLE_EVT */
    P2P_Client_App_Context.ConnectionHandle = pNotification->ConnectionHandle;
/* USER CODE END PEER_CONN_HANDLE_EVT */
      break;

    case PEER_DISCON_HANDLE_EVT :
/* USER CODE BEGIN PEER_DISCON_HANDLE_EVT */
      {
      uint8_t index = 0;
      P2P_Client_App_Context.ConnectionHandle =  0x00;
      P1_Disconnected();
      for (index=0; index<BLE_CFG_CLT_MAX_NBR_CB; ++index) {
        aP2PClientContext[index].FE43CharHandle=0;
        aP2PClientContext[index].FE43DescHandle=0;
        aP2PClientContext[index].p1_char_discovery_pending=0;
        aP2PClientContext[index].p1_char_discovery_failed=0;
        aP2PClientContext[index].p1_mtu_exchange_pending=0;
        aP2PClientContext[index].p1_mtu_exchange_done=0;
        aP2PClientContext[index].p1_negotiated_mtu=23;
      }
      index=0;
      while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].state != APP_BLE_IDLE))
      {
        aP2PClientContext[index].state = APP_BLE_IDLE;
      }
      BSP_LED_Off(LED_BLUE); 
        
#if OOB_DEMO == 0
      UTIL_SEQ_SetTask(1<<CFG_TASK_CONN_DEV_1_ID, CFG_SCH_PRIO_0);
#endif 
      }
/* USER CODE END PEER_DISCON_HANDLE_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Evt_Opcode_Default */

/* USER CODE END P2P_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN P2PC_APP_Notification_2 */

/* USER CODE END P2PC_APP_Notification_2 */
  return;
}
/* USER CODE BEGIN FD */
void P2PC_APP_SW1_Button_Action(void)
{

  UTIL_SEQ_SetTask(1<<CFG_TASK_SW1_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);

}
/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;

  P2P_Client_App_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch(blecore_evt->ecode)
      {

        case ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE:
        {
          aci_att_read_by_group_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t numServ, i, idx;
          uint16_t uuid, handle;

          uint8_t index;
          handle = pr->Connection_Handle;
          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].state != APP_BLE_IDLE))
          {
            APP_BLE_ConnStatus_t status;

            status = APP_BLE_Get_Client_Connection_Status(aP2PClientContext[index].connHandle);

            if((aP2PClientContext[index].state == APP_BLE_CONNECTED_CLIENT)&&
                    (status == APP_BLE_IDLE))
            {
              /* Handle deconnected */

              aP2PClientContext[index].state = APP_BLE_IDLE;
              aP2PClientContext[index].connHandle = 0xFFFF;
              break;
            }
            index++;
          }

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {
            aP2PClientContext[index].connHandle= handle;

            numServ = (pr->Data_Length) / pr->Attribute_Data_Length;

            /* the event data will be
             * 2bytes start handle
             * 2bytes end handle
             * 2 or 16 bytes data
             * we are interested only if the UUID is 16 bit.
             * So check if the data length is 6
             */
#if (UUID_128BIT_FORMAT==1)
          if (pr->Attribute_Data_Length == 20)
          {
            idx = 16;
#else
          if (pr->Attribute_Data_Length == 6)
          {
            idx = 4;
#endif
              for (i=0; i<numServ; i++)
              {
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx]);
                if(
#if (UUID_128BIT_FORMAT==1)
                  P1_UuidMatch(&pr->Attribute_Data_List[idx-12], P1_UUID_SERVICE)
#else
                  uuid == 0xFE40u
#endif
                )
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- P1 : FE40 SERVICE FOUND - connection handle 0x%x \n", aP2PClientContext[index].connHandle);
#endif
#if (UUID_128BIT_FORMAT==1)
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-16]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-14]);
#else
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-4]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-2]);
#endif
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_CHARACS ;
                }
                idx += pr->Attribute_Data_Length;
              }
            }
          }
        }
        break;

        case ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE:
        {

          aci_att_read_by_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t idx;
          uint16_t uuid, handle;

          /* the event data will be
           * 2 bytes start handle
           * 1 byte char properties
           * 2 bytes handle
           * 2 or 16 bytes data
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            /* Diagnostic parser for STM32WB ATT characteristic responses.
             * Confirm this event is actually delivered, before UUID filtering.
             * A record contains: declaration handle(2), properties(1),
             * value handle(2), UUID(2 or 16).  Typical size is 7 or 21.
             * Some stack revisions report Data_Length inclusive of a
             * record-length octet; account for both forms only when exact.
             */
            {
              uint16_t rec_len = pr->Handle_Value_Pair_Length;
              uint16_t data_len = pr->Data_Length;
              uint16_t payload_len = 0u;
              uint16_t offset = 0u;
              uint16_t n;
              APP_DBG_MSG("[P1 ATT] conn=0x%04X item_len=%u data_len=%u\r\n",
                pr->Connection_Handle, (unsigned)rec_len, (unsigned)data_len);

              if ((rec_len == 21u) || (rec_len == 7u))
              {
                if ((data_len > 0u) && (((data_len - 1u) % rec_len) == 0u))
                {
                  /* ST example: Data_Length=22 for a 21-byte record. */
                  payload_len = data_len - 1u;
                  APP_DBG_MSG("[P1 ATT] length format: prefix-inclusive\r\n");
                }
                else if ((data_len % rec_len) == 0u)
                {
                  payload_len = data_len;
                  APP_DBG_MSG("[P1 ATT] length format: payload-only\r\n");
                }
                else
                {
                  APP_DBG_MSG("[P1 ATT] malformed record length; skipping parser\r\n");
                }
                /* Raw byte logging limited to 63 bytes per event to avoid
                 * flooding VCP and holding the BLE event handler too long. */
                n = (payload_len < 63u) ? payload_len : 63u;
                APP_DBG_MSG("[P1 ATT RAW]");
                for (offset=0;offset<n;offset++)
                {
                  APP_DBG_MSG(" %02X",(unsigned)pr->Handle_Value_Pair_Data[offset]);
                }
                APP_DBG_MSG("\r\n");
                for (offset=0; (payload_len-offset)>=rec_len && rec_len != 0u; offset+=rec_len)
                {
                  const uint8_t *rec = &pr->Handle_Value_Pair_Data[offset];
                  uint16_t value_handle = UNPACK_2_BYTE_PARAMETER(&rec[3]);
                  uint16_t declaration_handle = UNPACK_2_BYTE_PARAMETER(&rec[0]);
                  uint8_t properties = rec[2];
                  if (rec_len == 21u)
                  {
                    const uint8_t *id = &rec[5];
                    APP_DBG_MSG("[P1 GATT] decl=0x%04X value=0x%04X props=0x%02X uuid=%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\r\n",
                      declaration_handle,value_handle,properties,id[15],id[14],id[13],id[12],id[11],id[10],id[9],id[8],id[7],id[6],id[5],id[4],id[3],id[2],id[1],id[0]);
                    if (P1_UuidMatch(id, P1_UUID_FE41))
                    {
                      aP2PClientContext[index].P2PWriteToServerCharHdle = value_handle;
                      APP_DBG_MSG("[P1] FE41 write handle=0x%04X\r\n",value_handle);
                    }
                    else if (P1_UuidMatch(id, P1_UUID_FE42))
                    {
                      aP2PClientContext[index].FE42CharHandle = value_handle;
                      APP_DBG_MSG("[P1] FE42 ctrl handle=0x%04X\r\n",value_handle);
                    }
                    else if (P1_UuidMatch(id, P1_UUID_FE43))
                    {
                      aP2PClientContext[index].FE43CharHandle = value_handle;
                      aP2PClientContext[index].P2PNotificationCharHdle = value_handle;
                      APP_DBG_MSG("[P1] FE43 stream handle=0x%04X\r\n",value_handle);
                    }
                  }
                  else
                  {
                    uint16_t uuid16=UNPACK_2_BYTE_PARAMETER(&rec[5]);
                    APP_DBG_MSG("[P1 GATT] decl=0x%04X value=0x%04X uuid16=0x%04X\r\n",declaration_handle,value_handle,uuid16);
                  }
                }
              }
              else
              {
                APP_DBG_MSG("[P1 ATT] unexpected item_len=%u\r\n",(unsigned)rec_len);
              }
            }
          }
        }
        break;

        case ACI_ATT_FIND_INFO_RESP_VSEVT_CODE:
        {
          aci_att_find_info_resp_event_rp0 *pr = (void*)blecore_evt->data;

          uint8_t numDesc, idx, i;
          uint16_t uuid, handle;

          /*
           * event data will be of the format
           * 2 bytes handle
           * 2 bytes UUID
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))

            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            numDesc = (pr->Event_Data_Length) / 4;
            /* we are interested only in 16 bit UUIDs */
            idx = 0;
            if (pr->Format == UUID_TYPE_16)
            {
              for (i=0; i<numDesc; i++)
              {
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx]);
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx+2]);

                if(uuid == CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID- connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  if( aP2PClientContext[index].state == APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC)
                  {

                    aP2PClientContext[index].P2PNotificationDescHandle = handle;
                    aP2PClientContext[index].FE43DescHandle = handle;
                    aP2PClientContext[index].state = APP_BLE_ENABLE_NOTIFICATION_DESC;

                  }
                }
                idx += 4;
              }
            }
          }
        }
        break; /*ACI_ATT_FIND_INFO_RESP_VSEVT_CODE*/

        /* ATT MTU response reports the agreed value. This is not the size
           of a single BLE link-layer PDU; max Notify payload is MTU - 3. */
        case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE:
        {
          aci_att_exchange_mtu_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t ix;
          for (ix=0;ix<BLE_CFG_CLT_MAX_NBR_CB;ix++)
          {
            if (aP2PClientContext[ix].connHandle == pr->Connection_Handle)
            {
              aP2PClientContext[ix].p1_negotiated_mtu = pr->Server_RX_MTU;
              APP_DBG_MSG("[P1 MTU] agreed=%u, notify_max=%u\r\n",
                (unsigned)pr->Server_RX_MTU,
                (unsigned)((pr->Server_RX_MTU>=3u)?(pr->Server_RX_MTU-3u):0u));
              break;
            }
          }
        }
        break;

        case ACI_GATT_NOTIFICATION_VSEVT_CODE:
        {
          aci_gatt_notification_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            if (pr->Attribute_Handle == aP2PClientContext[index].FE43CharHandle &&
                aP2PClientContext[index].FE43CharHandle != 0u)
            {
              P1_Notify_FE43(pr->Attribute_Value,pr->Attribute_Value_Length);
            }
          }
        }
        break;/* end ACI_GATT_NOTIFICATION_VSEVT_CODE */

        case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
        {
          aci_gatt_proc_complete_event_rp0 *pr = (void*)blecore_evt->data;
#if(CFG_DEBUG_APP_TRACE != 0)
          APP_DBG_MSG("-- GATT : ACI_GATT_PROC_COMPLETE_VSEVT_CODE \n");
          APP_DBG_MSG("\n");
#endif

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            if (aP2PClientContext[index].state == APP_BLE_DISCOVER_CHARACS)
            {
              /* After FE40 service discovery completes, the service response
                 has selected DISCOVER_CHARACS but no characteristic discovery
                 has been started yet. Schedule the next operation explicitly.
                 Otherwise the state machine stops immediately after FE40. */
              if (aP2PClientContext[index].p1_char_discovery_pending == 0u &&
                  aP2PClientContext[index].p1_char_discovery_failed == 0u)
              {
                APP_DBG_MSG("[P1] FE40 complete -> schedule characteristic discovery\r\n");
                UTIL_SEQ_SetTask(1<<CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);
              }
              else if (aP2PClientContext[index].p1_mtu_exchange_pending != 0u)
              {
                /* MTU exchange is its own GATT procedure: only after its
                   completion may descriptor discovery be initiated. */
                aP2PClientContext[index].p1_mtu_exchange_pending = 0u;
                aP2PClientContext[index].p1_mtu_exchange_done = 1u;
                APP_DBG_MSG("[P1 MTU] exchange complete; start FE43 CCCD discovery\r\n");
                aP2PClientContext[index].state = APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC;
                UTIL_SEQ_SetTask(1<<CFG_TASK_SEARCH_SERVICE_ID,CFG_SCH_PRIO_0);
              }
              else if (aP2PClientContext[index].p1_char_discovery_pending != 0u)
              {
                aP2PClientContext[index].p1_char_discovery_pending = 0u;
                if (aP2PClientContext[index].FE43CharHandle != 0u)
                {
                  tBleStatus mtu_status;
                  APP_DBG_MSG("[P1] FE43 FOUND; request MTU exchange (local max=%u)\r\n",
                              (unsigned)CFG_BLE_MAX_ATT_MTU);
                  mtu_status = aci_gatt_exchange_config(aP2PClientContext[index].connHandle);
                  APP_DBG_MSG("[P1 MTU] request status=0x%02X\r\n",(unsigned)mtu_status);
                  if (mtu_status == BLE_STATUS_SUCCESS)
                  {
                    aP2PClientContext[index].p1_mtu_exchange_pending = 1u;
                    /* Keep state DISCOVER_CHARACS until MTU procedure completes */
                  }
                  else
                  {
                    APP_DBG_MSG("[P1 MTU] request failed; trying CCCD with current MTU\r\n");
                    aP2PClientContext[index].state = APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC;
                    UTIL_SEQ_SetTask(1<<CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);
                  }
                }
                else
                {
                  APP_DBG_MSG("[P1] FE43 NOT FOUND; discovery stopped (no retry loop)\r\n");
                  aP2PClientContext[index].p1_char_discovery_failed = 1u;
                  aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
                }
              }
            }
            else
            {
              UTIL_SEQ_SetTask(1<<CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);
            }

          }
        }
        break; /*ACI_GATT_PROC_COMPLETE_VSEVT_CODE*/
        default:
          break;
      }
    }

    break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

    default:
      break;
  }

  return(return_value);
}/* end BLE_CTRL_Event_Acknowledged_Status_t */

void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification)
{
/* USER CODE BEGIN Gatt_Notification_1*/

/* USER CODE END Gatt_Notification_1 */
  switch(pNotification->P2P_Client_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Client_Evt_Opcode */

/* USER CODE END P2P_Client_Evt_Opcode */

    case P2P_NOTIFICATION_INFO_RECEIVED_EVT:
/* USER CODE BEGIN P2P_NOTIFICATION_INFO_RECEIVED_EVT */
    {
      P2P_Client_App_Context.LedControl.Device_Led_Selection=pNotification->DataTransfered.pPayload[0];
      switch(P2P_Client_App_Context.LedControl.Device_Led_Selection) {

        case 0x01 : {

          P2P_Client_App_Context.LedControl.Led1=pNotification->DataTransfered.pPayload[1];

          if(P2P_Client_App_Context.LedControl.Led1==0x00){
            BSP_LED_Off(LED_BLUE);
            APP_DBG_MSG(" -- P2P APPLICATION CLIENT : NOTIFICATION RECEIVED - LED OFF \n\r");
            APP_DBG_MSG(" \n\r");
          } else {
            APP_DBG_MSG(" -- P2P APPLICATION CLIENT : NOTIFICATION RECEIVED - LED ON\n\r");
            APP_DBG_MSG(" \n\r");
            BSP_LED_On(LED_BLUE);
          }

          break;
        }
        default : break;
      }

    }
/* USER CODE END P2P_NOTIFICATION_INFO_RECEIVED_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Client_Evt_Opcode_Default */

/* USER CODE END P2P_Client_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN Gatt_Notification_2*/

/* USER CODE END Gatt_Notification_2 */
  return;
}

uint8_t P2P_Client_APP_Get_State( void ) {
  return aP2PClientContext[0].state;
}
/* USER CODE BEGIN LF */
/**
 * @brief  Feature Characteristic update
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload)
{

  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint8_t index;

  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {

    switch(UUID)
    {
      case P2P_WRITE_CHAR_UUID: /* SERVER RX -- so CLIENT TX */
        ret =aci_gatt_write_without_resp(aP2PClientContext[index].connHandle,
                                         aP2PClientContext[index].P2PWriteToServerCharHdle,
                                         2, /* charValueLen */
                                         (uint8_t *)  pPayload);

        break;

      default:
        break;
    }
    index++;
  }

  return ret;
}/* end Write_Char() */

void Button_Trigger_Received(void)
{


  APP_DBG_MSG("-- P2P APPLICATION CLIENT  : BUTTON PUSHED - WRITE TO SERVER \n ");
  APP_DBG_MSG(" \n\r");
  if(P2P_Client_App_Context.ButtonStatus.Button1==0x00){
    P2P_Client_App_Context.ButtonStatus.Button1=0x01;
  }else {
    P2P_Client_App_Context.ButtonStatus.Button1=0x00;
  }

  Write_Char( P2P_WRITE_CHAR_UUID, 0, (uint8_t *)&P2P_Client_App_Context.ButtonStatus);

  return;
}

void Update_Service()
{
  uint16_t enable = 0x0001;
  uint16_t disable = 0x0000;

  uint8_t index;

  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {


    switch(aP2PClientContext[index].state)
    {

      case APP_BLE_DISCOVER_SERVICES:
        APP_DBG_MSG("[ANVIX P1] GATT UUID128 v0.6 diagnostics\n");
        APP_DBG_MSG("P2P_DISCOVER_SERVICES\n");
        break;
      case APP_BLE_DISCOVER_CHARACS:
        if (aP2PClientContext[index].p1_char_discovery_pending == 0u &&
            aP2PClientContext[index].p1_char_discovery_failed == 0u)
        {
          tBleStatus scan_result;
          APP_DBG_MSG("[P1] Characteristic discovery start: service=0x%04X..0x%04X\r\n",
            aP2PClientContext[index].P2PServiceHandle,
            aP2PClientContext[index].P2PServiceEndHandle);
          if (aP2PClientContext[index].P2PServiceHandle == 0u ||
              aP2PClientContext[index].P2PServiceEndHandle < aP2PClientContext[index].P2PServiceHandle)
          {
            APP_DBG_MSG("[P1] invalid FE40 service handle range\r\n");
            aP2PClientContext[index].p1_char_discovery_failed = 1u;
            aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
            break;
          }
          scan_result = aci_gatt_disc_all_char_of_service(aP2PClientContext[index].connHandle,
                         aP2PClientContext[index].P2PServiceHandle,
                         aP2PClientContext[index].P2PServiceEndHandle);
          APP_DBG_MSG("[P1] DISC_ALL_CHAR status=0x%02X\r\n",(unsigned)scan_result);
          if (scan_result == BLE_STATUS_SUCCESS)
            aP2PClientContext[index].p1_char_discovery_pending = 1u;
          else
          {
            APP_DBG_MSG("[P1] characteristic discovery start failed status=0x%02X\r\n",(unsigned)scan_result);
            aP2PClientContext[index].p1_char_discovery_failed = 1u;
            aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
          }
        }
        break;
      case APP_BLE_DISCOVER_WRITE_DESC: /* Not Used - No descriptor */
        APP_DBG_MSG("* GATT : Discover Descriptor of TX - Write  Characteristic\n");
        aci_gatt_disc_all_char_desc(aP2PClientContext[index].connHandle,
                                    aP2PClientContext[index].P2PWriteToServerCharHdle,
                                    aP2PClientContext[index].P2PWriteToServerCharHdle+2);

        break;
      case APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC:
        APP_DBG_MSG("[P1] Discover FE43 CCCD\n");
        aci_gatt_disc_all_char_desc(aP2PClientContext[index].connHandle,
                                    aP2PClientContext[index].P2PNotificationCharHdle,
                                    aP2PClientContext[index].P2PNotificationCharHdle+2);

        break;
      case APP_BLE_ENABLE_NOTIFICATION_DESC:
        APP_DBG_MSG("[P1] Enable FE43 Notification\n");
        aci_gatt_write_char_desc(aP2PClientContext[index].connHandle,
                                 aP2PClientContext[index].P2PNotificationDescHandle,
                                 2,
                                 (uint8_t *)&enable);

        aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
        BSP_LED_Off(LED_RED);

        break;
      case APP_BLE_DISABLE_NOTIFICATION_DESC :
        APP_DBG_MSG("* GATT : Disable Server Notification\n");
        aci_gatt_write_char_desc(aP2PClientContext[index].connHandle,
                                 aP2PClientContext[index].P2PNotificationDescHandle,
                                 2,
                                 (uint8_t *)&disable);

        aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;

        break;
      default:
        break;
    }
    index++;
  }
  return;
}
/* USER CODE END LF */

/* ANVIX adapter: use actual discovered GATT write handle. No fake success. */
int P1_Platform_BLE_Write_FE41(const uint8_t *data, uint16_t len)
{
  uint8_t index;
  if (!data || !len || len > 20u) return -1; /* default ATT MTU=23 */
  for (index = 0; index < BLE_CFG_CLT_MAX_NBR_CB; ++index)
  {
    if (aP2PClientContext[index].state == APP_BLE_CONNECTED_CLIENT &&
        aP2PClientContext[index].P2PWriteToServerCharHdle != 0u &&
        aP2PClientContext[index].connHandle != 0xFFFFu)
    {
      return (aci_gatt_write_without_resp(aP2PClientContext[index].connHandle,
               aP2PClientContext[index].P2PWriteToServerCharHdle,
               (uint8_t)len, (uint8_t *)data) == BLE_STATUS_SUCCESS) ? 0 : -2;
    }
  }
  return -3;
}
