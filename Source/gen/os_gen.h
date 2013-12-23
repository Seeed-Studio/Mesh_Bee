/****************************************************************************
 *
 *                 THIS IS A GENERATED FILE. DO NOT EDIT!
 *
 * MODULE:         OS
 *
 * COMPONENT:      os_gen.h
 *
 * DATE:           Wed Dec 18 13:34:58 2013
 *
 * AUTHOR:         Jennic RTOS Configuration Tool
 *
 * DESCRIPTION:    RTOS Application Configuration
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd. 2009 All rights reserved
 *
 ****************************************************************************/

#ifndef _OS_GEN_H
#define _OS_GEN_H

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define OS_STRICT_CHECKS

/* Module ZBPro */

/* Task Handles */
#define zps_taskZPS ((OS_thTask)&os_Task_zps_taskZPS)

/* Mutex Handles */
#define mutexZPS ((OS_thMutex)&os_Mutex_mutexZPS)
#define mutexPDUM ((OS_thMutex)&os_Mutex_mutexPDUM)
#define mutexMAC ((OS_thMutex)&os_Mutex_mutexMAC)
#define hSpiMutex ((OS_thMutex)&os_Mutex_hSpiMutex)

/* Message Handles */
#define zps_msgMlmeDcfmInd ((OS_thMessage)&os_Message_zps_msgMlmeDcfmInd)
#define zps_msgTimeEvents ((OS_thMessage)&os_Message_zps_msgTimeEvents)
#define zps_msgMcpsDcfmInd ((OS_thMessage)&os_Message_zps_msgMcpsDcfmInd)
#define zps_msgMlmeDcfmInd_C_Type MAC_tsMlmeVsDcfmInd
#define zps_msgTimeEvents_C_Type zps_tsTimeEvent
#define zps_msgMcpsDcfmInd_C_Type MAC_tsMcpsVsDcfmInd

/* Module ZBProAppCoordinator */

/* Task Handles */
#define APP_InitiateRejoin ((OS_thTask)&os_Task_APP_InitiateRejoin)
#define APP_taskHandleUartRx ((OS_thTask)&os_Task_APP_taskHandleUartRx)
#define APP_taskOTAReq ((OS_thTask)&os_Task_APP_taskOTAReq)
#define APP_AgeOutChildren ((OS_thTask)&os_Task_APP_AgeOutChildren)
#define APP_RadioRecal ((OS_thTask)&os_Task_APP_RadioRecal)

/* Cooperative Task Handles */
#define APP_taskMyEndPoint ((OS_thTask)&os_Task_APP_taskMyEndPoint)
#define APP_taskNWK ((OS_thTask)&os_Task_APP_taskNWK)

/* Mutex Handles */
#define mutexRxRb ((OS_thMutex)&os_Mutex_mutexRxRb)
#define mutexTxRb ((OS_thMutex)&os_Mutex_mutexTxRb)

/* Message Handles */
#define APP_msgZpsEvents ((OS_thMessage)&os_Message_APP_msgZpsEvents)
#define APP_msgMyEndPointEvents ((OS_thMessage)&os_Message_APP_msgMyEndPointEvents)
#define APP_msgZpsEvents_C_Type ZPS_tsAfEvent
#define APP_msgMyEndPointEvents_C_Type ZPS_tsAfEvent

/* Timer Handles */
#define APP_cntrTickTimer ((OS_thHWCounter)&os_HWCounter_APP_cntrTickTimer)
#define App_tmr1sec ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_App_tmr1sec)
#define APP_RouteRequestTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_RouteRequestTimer)
#define APP_tmrHandleUartRx ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_tmrHandleUartRx)
#define APP_JoinTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_JoinTimer)
#define APP_RejoinTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_RejoinTimer)
#define APP_OTAReqTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_OTAReqTimer)
#define APP_AgeOutChildrenTmr ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_AgeOutChildrenTmr)
#define APP_RadioRecalTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_RadioRecalTimer)

/* Module Exceptions */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef void (*OS_tprISR)(void);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/* Task Handles */
PUBLIC OS_thTask os_Task_zps_taskZPS;

/* Mutex Handles */
PUBLIC OS_thMutex os_Mutex_mutexZPS;
PUBLIC OS_thMutex os_Mutex_mutexPDUM;
PUBLIC OS_thMutex os_Mutex_mutexMAC;
PUBLIC OS_thMutex os_Mutex_hSpiMutex;

/* Message Handles */
PUBLIC OS_thMessage os_Message_zps_msgMlmeDcfmInd;
PUBLIC OS_thMessage os_Message_zps_msgTimeEvents;
PUBLIC OS_thMessage os_Message_zps_msgMcpsDcfmInd;

/* Task Handles */
PUBLIC OS_thTask os_Task_APP_InitiateRejoin;
PUBLIC OS_thTask os_Task_APP_taskHandleUartRx;
PUBLIC OS_thTask os_Task_APP_taskOTAReq;
PUBLIC OS_thTask os_Task_APP_AgeOutChildren;
PUBLIC OS_thTask os_Task_APP_RadioRecal;

/* Cooperative Task Handles */
PUBLIC OS_thTask os_Task_APP_taskMyEndPoint;
PUBLIC OS_thTask os_Task_APP_taskNWK;

/* Mutex Handles */
PUBLIC OS_thMutex os_Mutex_mutexRxRb;
PUBLIC OS_thMutex os_Mutex_mutexTxRb;

/* Message Handles */
PUBLIC OS_thMessage os_Message_APP_msgZpsEvents;
PUBLIC OS_thMessage os_Message_APP_msgMyEndPointEvents;

/* Timer Handles */
PUBLIC OS_thHWCounter os_HWCounter_APP_cntrTickTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_App_tmr1sec;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_RouteRequestTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_tmrHandleUartRx;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_JoinTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_RejoinTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_OTAReqTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_AgeOutChildrenTmr;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_RadioRecalTimer;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void OS_vStart(void (*)(void), void (*)(void), void (*)(OS_teStatus , void *));
PUBLIC OS_tprISR OS_prGetActiveISR(void);

PUBLIC bool os_bAPP_cbSetTickTimerCompare(uint32 );
PUBLIC uint32 os_u32APP_cbGetTickTimer(void);
PUBLIC void os_vAPP_cbEnableTickTimer(void);
PUBLIC void os_vAPP_cbDisableTickTimer(void);

#endif
