/**************************************************************************************************
  Filename:       EndDevice_Smoke.h
  Description:    Smoke sensor node - ADC MQ-2 on P0_7, buzzer on P1_2.
**************************************************************************************************/

#ifndef ENDDEVICE_SMOKE_H
#define ENDDEVICE_SMOKE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "hal_adc.h"

/*********************************************************************
 * CONSTANTS
 */

#define SAMPLEAPP_ENDPOINT           20

#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       5

#define SAMPLEAPP_PERIODIC_CLUSTERID   1
#define SAMPLEAPP_OKACK_CLUSTERID      2
#define SAMPLEAPP_TOGGLE_LED_CLUSTERID 3
#define SAMPLEAPP_SMOKE_CLUSTERID      6
#define SAMPLEAPP_CTRL_CLUSTERID       7

// Application Events (OSAL) - bit weighted
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001
#define SEND_SENSOR_DATA_EVENT                0x0002

// Group ID
#define SAMPLEAPP_FLASH_GROUP                  0x0001

// Smoke Sensor ADC
#define SMOKE_ADC_CH       HAL_ADC_CHANNEL_7   // 上海因仑 P0_7

// Buzzer Pin (P1_2)
#define BUZZER_BV          BV(3)
#define BUZZER_SBIT        P1_3  
#define BUZZER_DDR         P1DIR

// Smart Linkage Threshold
#define SMOKE_THRESHOLD    200

/*********************************************************************
 * FUNCTIONS
 */

extern void SampleApp_Init( uint8 task_id );
extern UINT16 SampleApp_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ENDDEVICE_SMOKE_H */
