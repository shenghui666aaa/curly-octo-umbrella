#include <stdio.h>
#include <string.h>

#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "Coordinator.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"

const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_OKACK_CLUSTERID,
  SAMPLEAPP_TOGGLE_LED_CLUSTERID,
  SAMPLEAPP_TEMP_HUMI_CLUSTERID,
  SAMPLEAPP_LIGHT_CLUSTERID,
  SAMPLEAPP_SMOKE_CLUSTERID,
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


static uint8 lastLightLow = 0;
static uint8 lastSmokeHigh = 0;

void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendCtrlCommand( uint8 cmd, uint8 value );


void SampleApp_Init( uint8 task_id )
{
  HalLcdInit();
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;

  MT_UartInit();
  MT_UartRegisterTaskID( SampleApp_TaskID );
  HalUARTWrite( 0, "Coordinator Ready\r\n", osal_strlen("Coordinator Ready\r\n") );

  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;

  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  afRegister( &SampleApp_epDesc );

  RegisterForKeys( SampleApp_TaskID );

  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7 );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

  {
  Clear_Screen(0, 0, 160, 128, BLACK);


  HalLcdWriteString( "智能环境检测", HAL_LCD_LINE_1, WHITE, BLACK );
  HalLcdWriteString( "马圣辉", HAL_LCD_LINE_3, WHITE, BLACK );
}
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
            HalLcdWriteString( "COORD Net OK", HAL_LCD_LINE_2, WHITE, BLACK );

            osal_start_timerEx( SampleApp_TaskID,
                               SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                               SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
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

  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    SampleApp_SendPeriodicMessage();

    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
       1000 );

    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  return 0;
}


void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
  (void)shift;

  if ( keys & HAL_KEY_SW_1 )
  {
    SampleApp_SendCtrlCommand( 0x01, 0x01 );
    HalLcdWriteString( "Cmd:LED ON", HAL_LCD_LINE_5, WHITE, BLACK );
    HalUARTWrite(0, "Manual: LED ON\r\n", osal_strlen("Manual: LED ON\r\n"));
  }

  if ( keys & HAL_KEY_SW_2 )
  {
    SampleApp_SendCtrlCommand( 0x02, 0x01 );
    HalLcdWriteString( "Cmd:Buz ON", HAL_LCD_LINE_5, WHITE, BLACK );
    HalUARTWrite(0, "Manual: Buzzer ON\r\n", osal_strlen("Manual: Buzzer ON\r\n"));
  }
}


void SampleApp_SendCtrlCommand( uint8 cmd, uint8 value )
{
  uint8 buf[2];
  afAddrType_t dstAddr;

  buf[0] = cmd;
  buf[1] = value;

  dstAddr.addrMode = afAddr16Bit;
  dstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  dstAddr.addr.shortAddr = 0xFFFF;

  AF_DataRequest( &dstAddr,
                  &SampleApp_epDesc,
                  SAMPLEAPP_CTRL_CLUSTERID,
                  2,
                  buf,
                  &SampleApp_TransID,
                  AF_DISCV_ROUTE,
                  AF_DEFAULT_RADIUS );

  HalUARTWrite( 0, "Ctrl sent\r\n", osal_strlen("Ctrl sent\r\n") );
}


void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
    uint8 str[10] = {0};
    char lcdStr[20] = {0};
    char uartStr[32] = {0};
    uint16 light, smoke;

    osal_memcpy(str, pkt->cmd.Data, pkt->cmd.DataLength);

    switch ( pkt->clusterId )
    {
        case SAMPLEAPP_OKACK_CLUSTERID:
            if (str[0] == 'O' && str[1] == 'K')
            {
                HalUARTWrite(0, "Ack:OK\r\n", osal_strlen("Ack:OK\r\n"));
                HalLedBlink(HAL_LED_1, 2, 50, 1000);
                HalLcdWriteString("Ack:OK", HAL_LCD_LINE_5, WHITE, BLACK);
            }
            break;

        case SAMPLEAPP_TOGGLE_LED_CLUSTERID:
            if ( pkt->cmd.DataLength >= 1 && pkt->cmd.Data[0] == 0x01 )
            {
                HalLedSet(HAL_LED_2, HAL_LED_MODE_TOGGLE);
                HalLcdWriteString("LED Toggled", HAL_LCD_LINE_5, WHITE, BLACK);
            }
            break;

        case SAMPLEAPP_TEMP_HUMI_CLUSTERID:
            if (pkt->cmd.DataLength >= 3 && str[0] == 'H')
            {
                sprintf(uartStr, "T=%d H=%d\r\n", pkt->cmd.Data[1], pkt->cmd.Data[2]);
                HalUARTWrite(0, (uint8*)uartStr, osal_strlen(uartStr));

                sprintf(lcdStr, "T=%d H=%d", pkt->cmd.Data[1], pkt->cmd.Data[2]);
                HalLcdWriteString(lcdStr, HAL_LCD_LINE_2, WHITE, BLACK);
            }
            break;

        // ========================
        // 已修好：光照正常显示 0~100，手遮会变化
        // ========================
        case SAMPLEAPP_LIGHT_CLUSTERID:
            if (pkt->cmd.DataLength >= 3 && str[0] == 'L')
            {
                // 直接取 0~100
                light = pkt->cmd.Data[2];

                sprintf(uartStr, "Light=%d\r\n", light);
                HalUARTWrite(0, (uint8*)uartStr, osal_strlen(uartStr));

                sprintf(lcdStr, "Light=%d", light);
                HalLcdWriteString(lcdStr, HAL_LCD_LINE_3, WHITE, BLACK);

                // 光照低 → 开灯
                if (light < 40 && !lastLightLow)
                {
                    lastLightLow = 1;
                    SampleApp_SendCtrlCommand(0x01, 0x01);
                    HalUARTWrite(0, "Auto: Light LOW -> LED ON\r\n",
                        osal_strlen("Auto: Light LOW -> LED ON\r\n"));
                }
                else if (light >= 40 && lastLightLow)
                {
                    lastLightLow = 0;
                    SampleApp_SendCtrlCommand(0x01, 0x00);
                    HalUARTWrite(0, "Auto: Light OK -> LED OFF\r\n",
                        osal_strlen("Auto: Light OK -> LED OFF\r\n"));
                }
            }
            break;

        case SAMPLEAPP_SMOKE_CLUSTERID:
            if (pkt->cmd.DataLength >= 3 && str[0] == 'S')
            {
               smoke = pkt->cmd.Data[2];

                sprintf(uartStr, "Smoke=%d\r\n", smoke);
                HalUARTWrite(0, (uint8*)uartStr, osal_strlen(uartStr));

                sprintf(lcdStr, "Smoke=%d", smoke);
                HalLcdWriteString(lcdStr, HAL_LCD_LINE_4, WHITE, BLACK);

                if (smoke > SMOKE_THRESHOLD && !lastSmokeHigh)
                {
                    lastSmokeHigh = 1;
                    SampleApp_SendCtrlCommand(0x02, 0x01);
                    HalUARTWrite(0, "Auto: Smoke HIGH -> Buzzer ON\r\n",
                        osal_strlen("Auto: Smoke HIGH -> Buzzer ON\r\n"));
                }
                else if (smoke <= SMOKE_THRESHOLD && lastSmokeHigh)
                {
                    lastSmokeHigh = 0;
                    SampleApp_SendCtrlCommand(0x02, 0x00);
                    HalUARTWrite(0, "Auto: Smoke OK -> Buzzer OFF\r\n",
                        osal_strlen("Auto: Smoke OK -> Buzzer OFF\r\n"));
                }
            }
            break;

        default:
            break;
    }
}


void SampleApp_SendPeriodicMessage( void )
{
  uint8 str[]="0123456789";

  afAddrType_t  SampleApp_Periodic_DstAddr;

  SampleApp_Periodic_DstAddr.addrMode = afAddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  AF_DataRequest( &SampleApp_Periodic_DstAddr,
                  &SampleApp_epDesc,
                  SAMPLEAPP_PERIODIC_CLUSTERID,
                  osal_strlen("0123456789"),
                  str,
                  &SampleApp_TransID,
                  AF_DISCV_ROUTE,
                  AF_DEFAULT_RADIUS );
}