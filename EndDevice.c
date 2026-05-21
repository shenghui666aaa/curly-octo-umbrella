#include <stdio.h>
#include <string.h>

#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "EndDevice.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"
#include "dht11.h"

const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_OKACK_CLUSTERID,
  SAMPLEAPP_TOGGLE_LED_CLUSTERID,
  SAMPLEAPP_TEMP_HUMI_CLUSTERID,
  SAMPLEAPP_CTRL_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,
  SAMPLEAPP_PROFID,
  SAMPLEAPP_DEVICEID,
  SAMPLEAPP_DEVICE_VERSION,
  SAMPLEAPP_FLAGS,
  SAMPLEAPP_MAX_CLUSTERS,
  (cId_t *)SampleApp_ClusterList,
  SAMPLEAPP_MAX_CLUSTERS,
  (cId_t *)SampleApp_ClusterList
};

endPointDesc_t SampleApp_epDesc;

uint8 SampleApp_TaskID;
devStates_t SampleApp_NwkState;
uint8 SampleApp_TransID;

afAddrType_t SampleApp_Flash_DstAddr;
aps_Group_t SampleApp_Group;

void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendSensorData( void );

void SampleApp_Init( uint8 task_id )
{
  HalLcdInit();
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;

  MT_UartInit();
  MT_UartRegisterTaskID( SampleApp_TaskID );
  HalUARTWrite( 0, "TempHumi Node Ready\r\n", osal_strlen("TempHumi Node Ready\r\n") );

  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  afRegister( &SampleApp_epDesc );

  RegisterForKeys( SampleApp_TaskID );

  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7 );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

  HalLcdWriteString( "TempHumi Node", HAL_LCD_LINE_1, WHITE, BLACK );
}

uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case KEY_CHANGE:
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (SampleApp_NwkState == DEV_ZB_COORD)
              || (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            HalLcdWriteString( "TempHumi Online", HAL_LCD_LINE_1, WHITE, BLACK );
            osal_start_timerEx( SampleApp_TaskID, SEND_SENSOR_DATA_EVENT, 2000 );
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & SEND_SENSOR_DATA_EVENT )
  {
    SampleApp_SendSensorData();
    osal_start_timerEx( SampleApp_TaskID, SEND_SENSOR_DATA_EVENT, 2000 );
    return (events ^ SEND_SENSOR_DATA_EVENT);
  }

  return 0;
}

void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
    (void)shift;

    if ( keys & HAL_KEY_SW_1 )
    {
        if ( SampleApp_NwkState != DEV_END_DEVICE )
        {
            HalLedBlink( HAL_LED_2, 2, 200, 500 );
            return;
        }

        uint8 cmd = 0x01;
        afAddrType_t dstAddr;
        dstAddr.addrMode = afAddr16Bit;
        dstAddr.endPoint = SAMPLEAPP_ENDPOINT;
        dstAddr.addr.shortAddr = 0x0000;

        AF_DataRequest( &dstAddr,
                        &SampleApp_epDesc,
                        SAMPLEAPP_TOGGLE_LED_CLUSTERID,
                        1,
                        &cmd,
                        &SampleApp_TransID,
                        AF_DISCV_ROUTE,
                        AF_DEFAULT_RADIUS );

        HalLedBlink( HAL_LED_2, 1, 50, 200 );
    }

    if ( keys & HAL_KEY_SW_2 )
    {
        HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
    }
}

void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  uint8 str[12];
  osal_memcpy(str, pkt->cmd.Data, pkt->cmd.DataLength);

  switch ( pkt->clusterId )
  {
    case SAMPLEAPP_PERIODIC_CLUSTERID:
      if (osal_memcmp(str, "0123456789", 10))
      {
        HalUARTWrite(0, str, pkt->cmd.DataLength);
        HalUARTWrite(0, "\r\n", 2);
        SampleApp_SendPeriodicMessage();
      }
      break;

    case SAMPLEAPP_CTRL_CLUSTERID:
      if (pkt->cmd.DataLength >= 2)
      {
        if (pkt->cmd.Data[0] == 0x01)
        {
          if (pkt->cmd.Data[1] == 0x01)
          {
            HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
            HalLcdWriteString("LED ON", HAL_LCD_LINE_5, WHITE, BLACK);
          }
          else
          {
            HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
            HalLcdWriteString("LED OFF", HAL_LCD_LINE_5, WHITE, BLACK);
          }
        }
        HalLcdWriteString("Ctrl from Coord", HAL_LCD_LINE_4, WHITE, BLACK);
      }
      break;

    default:
      break;
  }
}

void SampleApp_SendPeriodicMessage( void )
{
  uint8 str[]="OK";
  afAddrType_t  SampleApp_Periodic_DstAddr;

  SampleApp_Periodic_DstAddr.addrMode = afAddr16Bit;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0x0000;

  if ( AF_DataRequest( &SampleApp_Periodic_DstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_OKACK_CLUSTERID,
                       2,
                       str,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    HalLedBlink(HAL_LED_1, 1, 50, 500);
  }
  else
  {
    HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
  }
}

void SampleApp_SendSensorData( void )
{
    uint8 buf[3];
    uint8 temp = 0, humi = 0;
    char disStr[20];
    afAddrType_t dstAddr;

    buf[0] = 'H';

    // DHT11 读取（带重试）
    int8 retry;
    int8 result = -1;
    for (retry = 0; retry < 3; retry++)
    {
        result = dht11_value(&temp, &humi, 0);
        if(result == 0) break;
       
    }

    if (result == 0)
    {
        buf[1] = temp;
        buf[2] = humi;

        sprintf(disStr, "T=%d H=%d", temp, humi);
        HalLcdWriteString(disStr, HAL_LCD_LINE_2, WHITE, BLACK);

        sprintf(disStr, "T=%d H=%d\r\n", temp, humi);
        HalUARTWrite(0, (uint8*)disStr, osal_strlen(disStr));
    }
    else
    {
        buf[1] = 0;
        buf[2] = 0;
        HalLcdWriteString("DHT11 ERR!", HAL_LCD_LINE_2, WHITE, BLACK);
        HalUARTWrite(0, "DHT11 Read Error!\r\n", osal_strlen("DHT11 Read Error!\r\n"));
    }

    // 发送到协调器 0x0000
    dstAddr.addrMode = afAddr16Bit;
    dstAddr.endPoint = SAMPLEAPP_ENDPOINT;
    dstAddr.addr.shortAddr = 0x0000;

    if (AF_DataRequest(&dstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_TEMP_HUMI_CLUSTERID,
                       3,
                       buf,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
    {
        HalLedBlink(HAL_LED_1, 1, 50, 500);
    }
    else
    {
        HalUARTWrite(0, "Send Failed!\r\n", osal_strlen("Send Failed!\r\n"));
    }
}