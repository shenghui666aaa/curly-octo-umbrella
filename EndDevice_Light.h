/**************************************************************************************************
  Filename:       EndDevice_Light.h
  Description:    Light sensor node - ADC photoresistor on P0_5.
**************************************************************************************************/

#ifndef ENDDEVICE_LIGHT_H
#define ENDDEVICE_LIGHT_H

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
#define SAMPLEAPP_LIGHT_CLUSTERID      5
#define SAMPLEAPP_CTRL_CLUSTERID       7

// Application Events (OSAL) - bit weighted
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001
#define SEND_SENSOR_DATA_EVENT                0x0002

// Group ID
#define SAMPLEAPP_FLASH_GROUP                  0x0001

// Light Sensor ADC
#define LIGHT_ADC_CH       HAL_ADC_CHANNEL_7   // 上海因仑 P0_7

// Smart Linkage Threshold
#define LIGHT_THRESHOLD    50

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

#endif /* ENDDEVICE_LIGHT_H */
