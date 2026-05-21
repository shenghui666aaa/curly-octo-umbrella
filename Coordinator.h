/**************************************************************************************************
  Filename:       Coordinator.h
  Revised:        $Date: 2007-10-27 17:22:23 -0700 (Sat, 27 Oct 2007) $
  Revision:       $Revision: 15795 $

  Description:    Coordinator application definitions.

  Copyright 2007 Texas Instruments Incorporated. All rights reserved.
**************************************************************************************************/

#ifndef COORDINATOR_H
#define COORDINATOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * CONSTANTS
 */

#define SAMPLEAPP_ENDPOINT           20

#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       7

#define SAMPLEAPP_PERIODIC_CLUSTERID   1
#define SAMPLEAPP_OKACK_CLUSTERID      2
#define SAMPLEAPP_TOGGLE_LED_CLUSTERID 3
#define SAMPLEAPP_TEMP_HUMI_CLUSTERID  4
#define SAMPLEAPP_LIGHT_CLUSTERID      5
#define SAMPLEAPP_SMOKE_CLUSTERID      6
#define SAMPLEAPP_CTRL_CLUSTERID       7

// Send Message Timeout
#define SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT   5000

// Application Events (OSAL) - bit weighted
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001

// Group ID
#define SAMPLEAPP_FLASH_GROUP                  0x0001

// Flash Command Duration
#define SAMPLEAPP_FLASH_DURATION               1000

// Smart Linkage Thresholds (Coordinator auto-control)
#define LIGHT_THRESHOLD    50
#define SMOKE_THRESHOLD    50

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

#endif /* COORDINATOR_H */
