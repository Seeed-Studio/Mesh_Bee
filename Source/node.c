/*    
 * node.c
 * Firmware for SeeedStudio RFBeeV2(Zigbee) module 
 *   
 * Copyright (c) NXP B.V. 2012.   
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10 
 * Change Log :   
 *   
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.  
 */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "common.h"
#include "appapi.h"

#ifdef RADIO_RECALIBRATION
#include "recal.h"
#endif

#include "node.h"
#include "uart.h"
#include "endpoint.h"
#include "zigbee_join.h"
#include "app_ota.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_NODE
#define TRACE_NODE TRUE
#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

#ifdef PDM_EEPROM
PUBLIC uint8 u8PDM_CalculateFileSystemCapacity(void);
PUBLIC uint8 u8PDM_GetFileSystemOccupancy(void);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC PDM_tsRecordDescriptor   g_sDevicePDDesc;
PUBLIC tsDevice                 g_sDevice;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE uint8    u8ChildOfInterest = 0;
PRIVATE uint8    u8FailedRouteDiscoveries = 0;

//mac address which will be used when debug or manufactory.
//it will be place at .ro_mac_address section which can be 
//written by NXP flash programmer directly.
PRIVATE uint8 au8MacAddress[]__attribute__((section(".ro_mac_address"))) = {
#if defined(TARGET_COO)
    0xaa, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
#elif defined(TARGET_ROU)
    0xaa, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
#else
    0xaa, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
#endif
};

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vHandleConfigureNetworkEvent
 *
 * DESCRIPTION:
 * Handles stack events when the Controller is in its network configuration state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleConfigureNetworkEvent(ZPS_tsAfEvent sStackEvent)
{
    DBG_vPrintf(TRACE_NODE, "vHandleConfigureNetworkEvent \r\n");

    g_sDevice.eState = E_NETWORK_STARTUP;
    OS_eActivateTask(APP_taskNWK);
}

/****************************************************************************
 *
 * NAME: vHandleCOOStartupEvent
 *
 * DESCRIPTION:
 * Handle stack events when COO is in its network startup state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleCOOStartupEvent(ZPS_tsAfEvent sStackEvent)
{
    DBG_vPrintf(TRACE_NODE, "vHandleCOOStartupEvent \r\n");

    ZPS_teStatus eStatus = ZPS_eAplZdoStartStack();

    if (ZPS_E_SUCCESS == eStatus)
    {
        g_sDevice.eState = E_NETWORK_WAIT_FORMATION;
        OS_eActivateTask(APP_taskNWK);

        DBG_vPrintf(TRACE_NODE, "APP: Stack started\r\n");
    } else
    {
        DBG_vPrintf(TRACE_NODE_HIGH, "APP: ZPS_eZdoStartStack() failed error %d", eStatus);
        OS_eActivateTask(APP_taskNWK);
    }
}

/****************************************************************************
 *
 * NAME: vHandleNetworkFormationEvent
 *
 * DESCRIPTION:
 * Handles stack events when the Controller is in its network formation state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNetworkFormationEvent(ZPS_tsAfEvent sStackEvent)
{
    void *thisNet;
    ZPS_tsNwkNib *thisNib;

    /*wait for network stack to start up as a coordinator */
    if (ZPS_EVENT_NONE != sStackEvent.eType)
    {
        if (ZPS_EVENT_NWK_STARTED == sStackEvent.eType)
        {
            DBG_vPrintf(TRACE_NODE, "Network Started\r\n");

            thisNet = ZPS_pvAplZdoGetNwkHandle();
            thisNib = ZPS_psNwkNibGetHandle(thisNet);

            DBG_vPrintf(TRACE_NODE, "Channel: %d, PAN: 0x%x, ExPAN: 0x%0llx \r\n", thisNib->sPersist.u8VsChannel,
                        thisNib->sPersist.u16VsPanId, thisNib->sPersist.u64ExtPanId);

            /* turn on joining */
            ZPS_eAplZdoPermitJoining(0xff);

            g_sDevice.eState = E_NETWORK_RUN;
            PDM_vSaveRecord(&g_sDevicePDDesc);
            OS_eActivateTask(APP_taskNWK);
        } else
        {
            DBG_vPrintf(TRACE_NODE, "unexpected Event in E_NETWORK_STARTUP\r\n");
        }
    }
}


/****************************************************************************
 *
 * NAME: vHandleRunningEvent
 *
 * DESCRIPTION:
 * Forwards any event to the relevant event handler
 *
 *
 * PARAMETERS: Name         RW  Usage
 *             sAppEvent  R   Contains details of the app event
 *
 * RETURNS:
 * void
 * 
 * 
 ****************************************************************************/
PUBLIC void vHandleRunningEvent(ZPS_tsAfEvent sStackEvent)
{
    switch (sStackEvent.eType)
    {
    case ZPS_EVENT_NONE:
        break;
    case ZPS_EVENT_APS_DATA_INDICATION:
        break;
    case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
        break;
    case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
        break;
    case ZPS_EVENT_NWK_STARTED:
        break;
    case ZPS_EVENT_NWK_FAILED_TO_START:
        break;
    case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        break;
    case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
        DBG_vPrintf(TRACE_NODE, "ZPS_EVENT_NEW_NODE_HAS_JOINED\r\n");
        vDisplayNT();
        break;

    case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
        break;
    case ZPS_EVENT_NWK_LEAVE_INDICATION:
        break;
    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        break;
    case ZPS_EVENT_NWK_STATUS_INDICATION:
        DBG_vPrintf(TRACE_NODE, "STATUS_INDICATION: 0x%x from 0x%x.\r\n",
                    sStackEvent.uEvent.sNwkStatusIndicationEvent.u8Status,
                    sStackEvent.uEvent.sNwkStatusIndicationEvent.u16NwkAddr);
        break;
    case ZPS_EVENT_NWK_ROUTE_DISCOVERY_CONFIRM:
#ifdef TARGET_ROU
        DBG_vPrintf(TRACE_NODE, "Route Discovery Confirm with status: 0x%02x, ",
                    sStackEvent.uEvent.sNwkRouteDiscoveryConfirmEvent.u8Status);
        /* No route available */
        if (ZPS_NWK_ENUM_ROUTE_DISCOVERY_FAILED == sStackEvent.uEvent.sNwkRouteDiscoveryConfirmEvent.u8Status)
        {
            DBG_vPrintf(TRACE_NODE, "failed.\r\n");
            u8FailedRouteDiscoveries++;
            if (u8FailedRouteDiscoveries >= MAX_ROUTE_DISCOVERY_FAILURES)
            {
                u8FailedRouteDiscoveries = 0;
                OS_eActivateTask(APP_InitiateRejoin);
            }
        } else if (ZPS_NWK_ENUM_SUCCESS == sStackEvent.uEvent.sNwkRouteDiscoveryConfirmEvent.u8NwkStatus)
        {
            DBG_vPrintf(TRACE_NODE, "success.\r\n");
            u8FailedRouteDiscoveries = 0;
        }
#endif
        break;
    case ZPS_EVENT_APS_DATA_CONFIRM:
        break;
    case ZPS_EVENT_ERROR:
        break;
    case ZPS_EVENT_NWK_POLL_CONFIRM:
        break;
    case ZPS_EVENT_APS_ZDP_REQUEST_RESPONSE:
        {
#ifndef TARGET_END
#define NWK_RESP    0x8000
            if (NWK_RESP == sStackEvent.uEvent.sApsZdpEvent.u16ClusterId)
            {
                if (!sStackEvent.uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u8Status)
                {
                    ZPS_tsNwkNib *thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
                    uint8 i;

                    DBG_vPrintf(TRACE_NODE, "NWK Lookup Resp: 0x%04x 0x%016llx, age out.\r\n", sStackEvent.uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u16NwkAddrRemoteDev, sStackEvent.uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u64IeeeAddrRemoteDev);

                    for (i = 0; i < thisNib->sTblSize.u16NtActv; i++)
                    {
                        if ((sStackEvent.uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u16NwkAddrRemoteDev == thisNib->sTbl.psNtActv[i].u16NwkAddr) &&
                            (ZPS_NWK_NT_AP_RELATIONSHIP_CHILD == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship))
                        {
                            /* Remove child from the neighbour table */
                            thisNib->sTbl.psNtActv[i].u16NwkAddr = 0xFFFE;
                            thisNib->sTbl.psNtActv[i].u64ExtAddr = 0x0000000000000000ull;
                            thisNib->sTbl.psNtActv[i].u8LinkQuality = 0x00;
                            thisNib->sTbl.psNtActv[i].u8TxFailed = 0x00;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1Authenticated = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1DeviceType = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1ExpectAnnc = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1LinkStatusDone = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1PowerSource = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1RxOnWhenIdle = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1SecurityMode = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1Used = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u3Age = 0;
                            thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u3OutgoingCost = 0;
                        }
                    }
                } else
                {
                    DBG_vPrintf(TRACE_NODE, "Lookup Resp - Error: 0x%02x", sStackEvent.uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u8Status);
                }
            }
#endif
            break;
        }
    default:
        DBG_vPrintf(TRACE_NODE, "Unhandled event in vHandleRunningEvent: %x \r\n", sStackEvent.eType);
        break;
    }

}

/****************************************************************************
 *
 * NAME: refreshRoute
 *
 * DESCRIPTION:
 * For router, it will make sure the route towards coo periodly.
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void refreshRoute()
{
    if (OS_E_SWTIMER_RUNNING != OS_eGetSWTimerStatus(APP_RouteRequestTimer))
    {
        /* Send out a route request to the coordinator */
        ZPS_teStatus eStatus = ZPS_eAplZdoRouteRequest(
                               0x0000,
                               0x00);

        DBG_vPrintf(TRACE_NODE, "Route discovery sent with eStatus: 0x%02x\r\n", eStatus);

        /* Repeat in 60s */
        OS_eStartSWTimer(APP_RouteRequestTimer, APP_TIME_MS(60000), NULL);
    }
}



/****************************************************************************/
/***        Tasks                                                          ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_NWKTask
 *
 * DESCRIPTION:
 * Main State Machine for Meter Node
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskNWK)
{
    ZPS_tsAfEvent sStackEvent;

    sStackEvent.eType = ZPS_EVENT_NONE;

    DBG_vPrintf(TRACE_NODE, "\r\n-NWK-\r\n");

    if (OS_E_OK  == OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent))
    {
        DBG_vPrintf(TRACE_NODE, "Stack Evt: 0x%02x\r\n", sStackEvent.eType);

        if (sStackEvent.eType == ZPS_EVENT_ERROR)
        {
            DBG_vPrintf(TRACE_NODE, "ZigBee ERR: %x \r\n", sStackEvent.uEvent.sAfErrorEvent.eError);
        }
    }

    /* The main state machine for the Application */
    switch (g_sDevice.eState)
    {
        //config------------------------
    case E_NETWORK_CONFIG:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_CONFIG\r\n");
        vHandleConfigureNetworkEvent(sStackEvent);
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //start-up------------------------
    case E_NETWORK_STARTUP:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_STARTUP\r\n");
#ifdef TARGET_COO
        vHandleCOOStartupEvent(sStackEvent);
#else
        vHandleStartupEvent();
#endif
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //wait formation done------------------------
    case E_NETWORK_WAIT_FORMATION:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_WAIT_FORMATION\r\n");
        vHandleNetworkFormationEvent(sStackEvent);
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //discovery------------------------
    case E_NETWORK_DISCOVERY:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_DISCOVERY \r\n");
        vHandleNetworkDiscoveryEvent(sStackEvent);
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //joining------------------------
    case E_NETWORK_JOINING:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_JOINING \r\n");
        vHandleNetworkJoinEvent(sStackEvent);
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //init------------------------
    case E_NETWORK_INIT:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_INIT \r\n");

        break;
        //rescan
    case E_NETWORK_RESCAN:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_RESCAN \r\n");

        break;
        //wait leaving
    case E_NETWORK_WAIT_LEAVE:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_WAIT_LEAVE \r\n");
        vHandleNetworkLeave(sStackEvent);
        vAHI_DioSetOutput(0, (1 << DIO_ASSOC)); 
        break;
        //run------------------------
    case E_NETWORK_RUN:
        DBG_vPrintf(TRACE_NODE, "Handle State: E_NETWORK_RUN\r\n");
        vHandleRunningEvent(sStackEvent);
#ifdef TARGET_ROU
        //router should discovery the route to coo periodly.
        refreshRoute();
#endif
        vAHI_DioSetOutput((1 << DIO_ASSOC),0); 
        break;

    default:
        DBG_vPrintf(TRACE_NODE, "Unexpected state in NWK Task: %d\r\n", sStackEvent.eType);
        break;
    }
}

/****************************************************************************
 *
 * NAME: APP_InitiateRejoin
 *
 * DESCRIPTION:
 * Prepare the application and initiate a rejoin
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_InitiateRejoin)
{
    ZPS_teStatus eStatus = ZPS_eAplZdoRejoinNetwork();                            // Tell the stack to initiate a rejoin
    if (ZPS_E_SUCCESS != eStatus)
    {
        /* Stack not currently able to initiate
         * a rejoin, back off and try again later */
        DBG_vPrintf(TRACE_NODE, "Rejoin Error: %x, stack may already be rejoining\r\n", eStatus);
        OS_eStartSWTimer(APP_RejoinTimer, APP_TIME_MS(1000), NULL);
    } else
    {
        bRejoining = TRUE;                                                            // Set the rejoin flag
        g_sDevice.eState = E_NETWORK_JOINING;                                        // Set the state machine to handle a rejoin event
        PDM_vSaveRecord(&g_sDevicePDDesc);
    }
}

/****************************************************************************
 *
 * NAME: APP_AgeOutChildren
 *
 * DESCRIPTION:
 * Cycles through the device's children on a context restore to find out if
 * they now have a different parent
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_AgeOutChildren)
{
#ifndef TARGET_END
    ZPS_tsNwkNib *thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
    uint8 i;

    for (i = u8ChildOfInterest; i < thisNib->sTblSize.u16NtActv; i++)
    {
        if (ZPS_NWK_NT_AP_RELATIONSHIP_CHILD == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship)
        {
            /* Child found in the neighbour table, send out a address request.
             * If anyone responds we know to age out the child */
            u8ChildOfInterest = i;

            PDUM_thAPduInstance hAPduInst;
            hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

            if (hAPduInst == PDUM_INVALID_HANDLE)
            {
                DBG_vPrintf(TRACE_NODE, "IEEE Address Request - PDUM_INVALID_HANDLE\r\n");
            } else
            {
                uint8 u8TransactionSequenceNumber;

                /* Broadcast to all Rx-On-When-Idle devices */
                ZPS_tuAddress uAddress;
                uAddress.u16Addr = 0xFFFD;

                ZPS_tsAplZdpNwkAddrReq sAplZdpNwkAddrReq;
                sAplZdpNwkAddrReq.u64IeeeAddr = thisNib->sTbl.psNtActv[u8ChildOfInterest].u64ExtAddr;
                sAplZdpNwkAddrReq.u8RequestType = 0;

                DBG_vPrintf(TRACE_NODE, "Child found in NT, sending route request: 0x%04x\r\n", thisNib->sTbl.psNtActv[u8ChildOfInterest].u16NwkAddr);
                ZPS_teStatus eStatus = ZPS_eAplZdpNwkAddrRequest(hAPduInst,
                                                                 uAddress,
                                                                 FALSE,
                                                                 &u8TransactionSequenceNumber,
                                                                 &sAplZdpNwkAddrReq
                                                                );

                if (eStatus)
                {
                    DBG_vPrintf(TRACE_NODE, "Address Request failed: 0x%02x\r\n", eStatus);
                } else
                {
                    u8ChildOfInterest++;
                    break;
                }
            }
        }
    }

    if (i >= thisNib->sTblSize.u16NtActv)
    {
        if (OS_eGetSWTimerStatus(APP_AgeOutChildrenTmr) != OS_E_SWTIMER_STOPPED)
        {
            OS_eStopSWTimer(APP_AgeOutChildrenTmr);                                 // No children left in the NT to query for
        }
        u8ChildOfInterest = 0;
    } else
    {
        OS_eStartSWTimer(APP_AgeOutChildrenTmr, APP_TIME_MS(1600), NULL);        // Re-activate this task in 1.6s to scan for the next child
    }
#endif
}


/****************************************************************************
 *
 * NAME: APP_RadioRecal
 *
 * DESCRIPTION:
 * Recalibrate the radio 
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_RadioRecal)
{
#ifdef RADIO_RECALIBRATION
    if (OS_E_SWTIMER_EXPIRED == OS_eGetSWTimerStatus(APP_RadioRecalTimer))
    {
        DBG_vPrintf(TRACE_NODE, "Recalibrate the radio\r\n");
        uint8 eStatus = eAHI_AttemptCalibration();
        if (eStatus)
        {
            DBG_vPrintf(TRACE_NODE, "Recalibration already underway");
            OS_eStartSWTimer(APP_RadioRecalTimer, APP_TIME_SEC(1), NULL);            // Re-activate this task in 1s
        } else
        {
            OS_eStartSWTimer(APP_RadioRecalTimer, APP_TIME_SEC(120), NULL);        // Re-activate this task in 2min
                                                                                   //(u32Ticks max value 0x7fffffff ~=134sec)
        }
    }
#endif
}



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: initDeviceDefault
 *
 * DESCRIPTION:
 * init the default value for tsDevice global config.
 *
 * PARAMETERS: Name         RW  Usage
 *             dev          RW  pointer to tsDevice struct
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void initDeviceDefault(tsDevice *dev)
{
    dev->eState = E_NETWORK_CONFIG;
    dev->eSubState = E_SUB_NONE;
    dev->eMode  = E_MODE_DATA;
    dev->magic  = PDM_REC_MAGIC;
    dev->len    = sizeof(tsDevice);

    dev->supportOTA = FALSE;
    dev->isOTASvr   = FALSE;
    dev->otaTotalBlocks = 0;
    dev->otaTotalBytes  = 0;
#ifdef OTA_CLIENT
    dev->otaDownloading = 0;
    dev->otaCurBlock = 0;
    dev->otaReqPeriod = 1000;
    dev->otaSvrAddr16 = 0x0;
#endif

    dev->config.txMode = BROADCAST;
    dev->config.autoJoinFirst = 1;
    dev->config.baudRateUart1 = E_AHI_UART_RATE_115200;
    dev->config.powerUpAction = 1;
    dev->config.reqPeriodMs   = 1000;
}

/****************************************************************************
 *
 * NAME: deleteStackPDM
 *
 * DESCRIPTION:
 * delete the stack part of PDM record.
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void deleteStackPDM()
{
    tsDevice backup;
    memcpy(&backup, &g_sDevice, sizeof(backup));
    PDM_vDelete();
    PDM_eLoadRecord(&g_sDevicePDDesc, REC_ID1, &g_sDevice, sizeof(g_sDevice), FALSE);
    memcpy(&g_sDevice, &backup, sizeof(backup));
    PDM_vSaveRecord(&g_sDevicePDDesc);
}

/****************************************************************************
 *
 * NAME: node_vInitialise
 *
 * DESCRIPTION:
 * Initialises the node application
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void node_vInitialise(void)
{
    PDM_eLoadRecord(&g_sDevicePDDesc, REC_ID1, &g_sDevice, sizeof(g_sDevice), FALSE);
    if (g_sDevice.magic != PDM_REC_MAGIC || g_sDevice.len != sizeof(g_sDevice))
    {
        DBG_vPrintf(TRACE_NODE, "pdm record magic not match\r\n");
        PDM_vDelete();
        initDeviceDefault(&g_sDevice);
        PDM_eLoadRecord(&g_sDevicePDDesc, REC_ID1, &g_sDevice, sizeof(g_sDevice), FALSE);
    }
    //if configed powerup actions non-zero, then node should redo the network related stuff.
    if (g_sDevice.config.powerUpAction)
    {
        DBG_vPrintf(TRACE_NODE, "Re-form/re-scan network...\r\n");
        deleteStackPDM();
        g_sDevice.eState = E_NETWORK_CONFIG;
        g_sDevice.config.powerUpAction = 0;
        PDM_vSaveRecord(&g_sDevicePDDesc);
    }

    if (E_NETWORK_CONFIG == g_sDevice.eState) // If context is blank
    {
        // set the security state default link key 
        //ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, s_au8LnkKeyArray,
        //                                   0, ZPS_APS_UNIQUE_LINK_KEY );
    }

    // during developing, we can overwrite the on-chip mac addr 
    //ZPS_vSetOverrideLocalMacAddress((uint64 *)&au8MacAddress);

    //Initialise ZBPro stack 
    ZPS_eAplAfInit();


    DBG_vPrintf(TRACE_NODE, "PDM: Capacity %d\r\n", u8PDM_CalculateFileSystemCapacity());
    DBG_vPrintf(TRACE_NODE, "PDM: Occupancy %d\r\n", u8PDM_GetFileSystemOccupancy());


    // Cant through the compiling, vAHI_ETSIHighPowerModuleEnable not found,
    // what the poor documents nxp supplies!
    //if (g_sDevice.config.etsi)
    //{
    //    DBG_vPrintf(TRACE_NODE, "Limit the module to +10dbm for ETSI compliance \r\n");
    //    vAHI_ETSIHighPowerModuleEnable(TRUE); // Limit the module to +8dB for ETSI compliance
    //}

    // Initialise  
    endpoint_vInitialize();
    uart_initialize();

    // If the device state has been restored from eep, re-start the stack
    //  and set the application running again 
    if (g_sDevice.eState > E_NETWORK_STARTUP) // If the last known state was a running state (i.e. a context restore)
    {
        ZPS_eAplZdoStartStack();
        DBG_vPrintf(TRACE_NODE, "Restoring Context, app state %d, \r\n", g_sDevice.eState);

#ifndef TARGET_COO
        vRestoreLastNWK(&g_sDevice.nwDesc);
#endif

#ifndef TARGET_END
        ZPS_eAplZdoPermitJoining(0xff);
        // Activate the child aging task on a context restore to make sure any previous
        // children haven't jumped to another parent whilst we were offline 
        OS_eActivateTask(APP_AgeOutChildren);
#endif

#ifdef OTA_CLIENT
        if (g_sDevice.otaDownloading > 0) OS_eActivateTask(APP_taskOTAReq);
#endif
    }
    // else perform any actions required on initial start-up 
    else
    {
        ZPS_eAplZdoPermitJoining(0xff);
        //g_sDevice.bPermitJoining = TRUE;
    }

    // Start the tick timer
    //OS_eStartSWTimer(App_tmr1sec, ONE_SECOND_TICK_TIME, NULL);

    // Activate the radio recalibration task in 60s 
#ifdef RADIO_RECALIBRATION
    OS_eStartSWTimer(APP_RadioRecalTimer, APP_TIME_SEC(60), NULL);
#endif

    // OTA 
#ifdef CLD_OTA
    DBG_vPrintf(TRACE_NODE, "Initializing OTA.\r\n");
    g_sDevice.supportOTA = TRUE;
    bool bIsServer = FALSE;
#ifdef OTA_SERVER
    bIsServer = TRUE;
#endif
    g_sDevice.isOTASvr = bIsServer;
#ifndef FACT_TEST
    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);
#endif

#endif

    OS_eActivateTask(APP_taskNWK);
    
    //light on on/sleep led.
    vAHI_DioSetDirection(0, (1 << DIO_ON_SLEEP));
    vAHI_DioSetOutput((1 << DIO_ON_SLEEP), 0);
    
    #ifndef FACT_TEST
    //init the association led pin
    vAHI_DioSetDirection(0, (1 << DIO_ASSOC)); 
    
    //init pwm for rssi
    vAHI_TimerEnable(E_AHI_TIMER_1, 4, FALSE, FALSE, TRUE);
    vAHI_TimerStartRepeat(E_AHI_TIMER_1, 1000, 1);
    #endif
}



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
