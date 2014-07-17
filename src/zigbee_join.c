/*
 * zigbee_join.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log : [2014/04 oliver] support for API network operation
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

#include <stdio.h>
#include <rnd_pub.h>
#include "common.h"
#include "zigbee_join.h"
#include "firmware_at_api.h"
#include "firmware_uart.h"
#include "zigbee_node.h"
#include "firmware_api_pack.h"
#include "firmware_cmi.h"

#ifndef TRACE_JOIN
#define TRACE_JOIN   FALSE
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
PUBLIC bool bRejoining = FALSE;


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE ZPS_tsNwkNetworkDescr tsDiscovedNWKList[MAX_SINGLE_CHANNEL_NETWORKS];
PRIVATE uint8 u8DiscovedNWKListCount = 0;
PRIVATE uint8 u8DiscovedNWKJoinCount = 0;
PRIVATE uint8 u8LocalChannelMask = 11;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 * NAME: AT_reScanNetwork
 *
 * DESCRIPTION:
 * At command action function
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   useless
 *
 * RETURNS:
 * int: OK - at action success
 ****************************************************************************/
int AT_reScanNetwork(uint16 *regAddr)
{
	/* Leave Nwk at first */
    ZPS_teStatus e = ZPS_eAplZdoLeaveNetwork(0, FALSE, FALSE);
    if (e)
    {
    	/* Failed to leave,restart */
        DBG_vPrintf(TRACE_JOIN, "Leave failed.\r\n");
        g_sDevice.eState = E_NETWORK_STARTUP;
        deleteStackPDM();
        vAHI_SwReset();
        return OK;
    } else
    {
    	/* Reset Nwk stack State Machine */
        DBG_vPrintf(TRACE_JOIN, "Wait leaving...\r\n");
        g_sDevice.eSubState = E_SUB_RESCANNING;
        vStartStopTimer(APP_JoinTimer, APP_TIME_MS(5000), E_NETWORK_WAIT_LEAVE);
        return OK;
    }
}



int API_RescanNetwork_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
    int ret = AT_reScanNetwork(regAddr);
    int resp = (ret == OK) ? AT_OK:AT_ERR;

    if (API_LOCAL_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsLocalAtResp localAtResp;
        int len = assembleLocalAtResp(&localAtResp,
                                      inputApiSpec->payload.localAtReq.frameId,
                                      inputApiSpec->payload.localAtReq.atCmdId,
                                      resp, (uint8 *)&resp, 2);
        assembleApiSpec(retApiSpec, API_LOCAL_AT_RESP, (uint8 *)&localAtResp, len);
    } else if (API_REMOTE_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsRemoteAtResp remoteAtResp;
        int len = assembleRemoteAtResp(&remoteAtResp,
                                       inputApiSpec->payload.remoteAtReq.frameId,
                                       inputApiSpec->payload.remoteAtReq.atCmdId,
                                       resp, (uint8 *)&resp, 2, inputApiSpec->payload.remoteAtReq.unicastAddr);
        assembleApiSpec(retApiSpec, API_REMOTE_AT_RESP, (uint8 *)&remoteAtResp, len);
    }
    return OK;
}

/****************************************************************************
 * NAME: AT_reJoinNetwork
 *
 * DESCRIPTION:
 * rejoin the last network
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   useless
 *
 * RETURNS:
 * int: OK - at action success
 ****************************************************************************/

int AT_reJoinNetwork(uint16 *regAddr)
{
    /* Leave Nwk with rejoin */
    ZPS_teStatus e = ZPS_eAplZdoLeaveNetwork(0, FALSE, TRUE);
    if (e)
    {
        /* Failed to leave,restart */
        DBG_vPrintf(TRACE_JOIN, "Leave failed.\r\n");
        g_sDevice.eState = E_NETWORK_JOINING; 
        ZPS_eAplZdoRejoinNetwork();
        return OK;
    } else
    {
        /* Reset Nwk stack State Machine */
        DBG_vPrintf(TRACE_JOIN, "Wait leaving...\r\n");
        g_sDevice.eSubState = E_SUB_REJOINNING; 
        vStartStopTimer(APP_JoinTimer, APP_TIME_MS(5000), E_NETWORK_WAIT_LEAVE);
        return OK;
    }
}
/****************************************************************************
 * NAME: vHandleNetworkLeave
 *
 * DESCRIPTION:
 * handle stack event when node is in its network leave state
 *
 * PARAMETERS: Name         RW  Usage
 *
 * RETURNS:
 * void
 ****************************************************************************/
PUBLIC void vHandleNetworkLeave(ZPS_tsAfEvent sStackEvent)
{
    uint8 endUp = 0;

    if (ZPS_EVENT_NONE != sStackEvent.eType)
    {
        switch (sStackEvent.eType)
        {

        case ZPS_EVENT_NWK_LEAVE_CONFIRM:
            DBG_vPrintf(TRACE_JOIN, "Left network.\r\n");
            if (OS_eGetSWTimerStatus(APP_JoinTimer) != OS_E_SWTIMER_STOPPED)
            {
                OS_eStopSWTimer(APP_JoinTimer);
            }
            endUp = 1;
            break;
        default:
            break;
        }
    } else
    {
        if (OS_eGetSWTimerStatus(APP_JoinTimer) == OS_E_SWTIMER_EXPIRED) endUp = 2;
    }

    //handle re-scan's 2nd stage
    if (g_sDevice.eSubState == E_SUB_RESCANNING && endUp > 0)
    {
        g_sDevice.eState = E_NETWORK_STARTUP;
        g_sDevice.eSubState = E_SUB_NONE;
        deleteStackPDM();
        vAHI_SwReset();
    }
    
    //handle re-join's 2nd stage
    if (g_sDevice.eSubState == E_SUB_REJOINNING && endUp == 1)
    {
        g_sDevice.eState = E_NETWORK_JOINING;
        g_sDevice.eSubState = E_SUB_NONE;
    }
    if (g_sDevice.eSubState == E_SUB_REJOINNING && endUp == 2) 
    {
        g_sDevice.eState = E_NETWORK_JOINING;
        ZPS_eAplZdoRejoinNetwork(); 
    }
    
}

/****************************************************************************
 * NAME: AT_joinNetworkWithIndex
 *
 * DESCRIPTION:
 * at command action function
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   pointer to a uint16 that indicating the index of network to join
 *
 * RETURNS:
 * void
 ****************************************************************************/
int AT_joinNetworkWithIndex(uint16 *regAddr)
{
    uint16 idx = *regAddr;

    if (idx > u8DiscovedNWKListCount || u8DiscovedNWKListCount == 0)
    {
        DBG_vPrintf(TRACE_JOIN, "Illegal index!\r\n");
        return ERR;
    }

    u8DiscovedNWKJoinCount = idx;

    char tmp[90];
    int len;

    if (g_sDevice.eState > E_NETWORK_JOINING)
    {
        len = sprintf(tmp, "Due to the shortness of stack, you should re-scan with 'ATRS' cmd.\r\n");
        uart_tx_data(tmp, len);
        DBG_vPrintf(TRACE_JOIN, "%s", tmp);
        return ERR;
    }
    else
    {
        len = sprintf(tmp, "Joining PAN: %08x%08x \r\n",
                      (uint32)(tsDiscovedNWKList[u8DiscovedNWKJoinCount].u64ExtPanId >> 32),
                      (uint32)(tsDiscovedNWKList[u8DiscovedNWKJoinCount].u64ExtPanId)
                     );
        uart_tx_data(tmp, len);
        DBG_vPrintf(TRACE_JOIN, "%s", tmp);

        ZPS_teStatus eStatus = ZPS_eAplZdoJoinNetwork(&tsDiscovedNWKList[u8DiscovedNWKJoinCount]);

        if (ZPS_E_SUCCESS == eStatus)
        {
            len = sprintf(tmp, "Joined in.\r\n");
            uart_tx_data(tmp, len);
            DBG_vPrintf(TRACE_JOIN, "%s", tmp);
            g_sDevice.eState = E_NETWORK_JOINING;
            u8DiscovedNWKJoinCount++;
            return OK;
        }
        else
        {
            len = sprintf(tmp, "Join ERR: %x\r\n", eStatus);
            uart_tx_data(tmp, len);
            DBG_vPrintf(TRACE_JOIN, "%s", tmp);
            u8DiscovedNWKJoinCount++;
            return ERR;
        }
    }

}




int API_JoinNetworkWithIndex_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
    //save the register
    if (inputApiSpec->payload.localAtReq.value[0] != 0)
    {
        memcpy((uint8 *)regAddr, inputApiSpec->payload.localAtReq.value + 1, 2);
        PDM_vSaveRecord(&g_sDevicePDDesc);
    }

    int result = 0;
    char tmp[90];
    int len;
    do
    {
        uint16 idx = *regAddr;

        if (idx > u8DiscovedNWKListCount || u8DiscovedNWKListCount == 0)
        {
            len = sprintf(tmp, "Illegal Index\r\n");
            DBG_vPrintf(TRACE_JOIN, "%s", tmp);
            result = 1;
            break;
        }

        u8DiscovedNWKJoinCount = idx;


        if (g_sDevice.eState > E_NETWORK_JOINING)
        {
            len = sprintf(tmp, "Need ATRS\r\n");
            DBG_vPrintf(TRACE_JOIN, "%s", tmp);
            result = 2;
            break;
        } else
        {
            len = sprintf(tmp, "Joining PAN: %08x%08x \r\n",
                          (uint32)(tsDiscovedNWKList[u8DiscovedNWKJoinCount].u64ExtPanId >> 32),
                          (uint32)(tsDiscovedNWKList[u8DiscovedNWKJoinCount].u64ExtPanId)
                         );
            DBG_vPrintf(TRACE_JOIN, "%s", tmp);

            ZPS_teStatus eStatus = ZPS_eAplZdoJoinNetwork(&tsDiscovedNWKList[u8DiscovedNWKJoinCount]);

            if (ZPS_E_SUCCESS == eStatus)
            {
                len = sprintf(tmp, "Joined in.\r\n");
                DBG_vPrintf(TRACE_JOIN, "%s", tmp);
                g_sDevice.eState = E_NETWORK_JOINING;
                u8DiscovedNWKJoinCount++;
                result = 0;
                break;
            } else
            {
                len = sprintf(tmp, "Join ERR: %x\r\n", eStatus);
                DBG_vPrintf(TRACE_JOIN, "%s", tmp);
                u8DiscovedNWKJoinCount++;
                result = 3;
                break;
            }
        }
    } while (false);

    int resp = (result == 0) ? AT_OK : AT_ERR;

    if (API_LOCAL_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsLocalAtResp localAtResp;
        int size = assembleLocalAtResp(&localAtResp,
                                      inputApiSpec->payload.localAtReq.frameId,
                                      inputApiSpec->payload.localAtReq.atCmdId,
                                      resp, tmp, len);
        assembleApiSpec(retApiSpec, API_LOCAL_AT_RESP, (uint8 *)&localAtResp, size);
    } else if (API_REMOTE_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsRemoteAtResp remoteAtResp;
        int size = assembleRemoteAtResp(&remoteAtResp,
                                       inputApiSpec->payload.remoteAtReq.frameId,
                                       inputApiSpec->payload.remoteAtReq.atCmdId,
                                       resp, tmp, len, inputApiSpec->payload.remoteAtReq.unicastAddr);
        assembleApiSpec(retApiSpec, API_REMOTE_AT_RESP, (uint8 *)&remoteAtResp, size);
    }
    return OK;
}


/****************************************************************************
 *
 * NAME: vHandleStartupEvent
 *
 * DESCRIPTION:
 * Handles stack events when node(not Coo) is in its startup state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleStartupEvent(void)
{
    ZPS_teStatus eStatus;

    ZPS_tsNwkNib *thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    if (OS_E_SWTIMER_RUNNING != OS_eGetSWTimerStatus(APP_JoinTimer))
    {
        // Clear the discovery neighbour table on each scan
        vClearDiscNT();

        eStatus = ZPS_eAplZdoStartStack();

        if (eStatus)
        {
            DBG_vPrintf(TRACE_JOIN, "StartStack: ERR: %x \r\n", eStatus);
            vStartStopTimer(APP_JoinTimer, APP_TIME_MS(100), E_NETWORK_STARTUP);
        } else
        {
            g_sDevice.eState = E_NETWORK_DISCOVERY;
        }
    } // OS_E_SWTIMER_RUNNING != OS_eGetSWTimerStatus(APP_RestartTimer)
}



/****************************************************************************
 *
 * NAME: vHandleNetworkDiscoveryEvent
 *
 * DESCRIPTION:
 * Handles stack events when the Controller is in its network discovery state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkDiscoveryEvent(ZPS_tsAfEvent sStackEvent)
{
    /* wait for node to discover networks... */
    if (ZPS_EVENT_NONE != sStackEvent.eType)
    {
        switch (sStackEvent.eType)
        {

        case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:

            DBG_vPrintf(TRACE_JOIN, "Discovery Complete\r\n");

            if ((ZPS_E_SUCCESS != sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus))
            {
                DBG_vPrintf(TRACE_JOIN, "Discovery ERR: 0x%x, \r\n", sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus);
                vStartStopTimer(APP_JoinTimer, APP_TIME_MS(1000), E_NETWORK_STARTUP);
                return;
            }

            if ((sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount == 0))
            {
                DBG_vPrintf(TRACE_JOIN, "Discovered no network. \r\n");
                vStartStopTimer(APP_JoinTimer, APP_TIME_MS(1000), E_NETWORK_STARTUP);
                return;
            }

            DBG_vPrintf(TRACE_JOIN, "Scanned NWK: %d\r\n", sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount);


            vSaveDiscoveredNWKS(sStackEvent);
            vDisplayDiscoveredNWKS();
            if (g_sDevice.config.autoJoinFirst)
            {
                DBG_vPrintf(TRACE_JOIN, "Auto join the recommand network. \r\n");
                vJoinStoredNWK();
            }
            break;

        case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
            vJoinedNetwork(sStackEvent);
            break;

        case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
            vJoinedNetwork(sStackEvent);
            break;

        case ZPS_EVENT_NWK_FAILED_TO_JOIN:
            DBG_vPrintf(TRACE_JOIN, "Failed to join: Join ERR = 0x%x\r\n",    sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status);
            //u8DiscovedNWKJoinCount++;
            //vJoinStoredNWK();
            vStartStopTimer(APP_JoinTimer, APP_TIME_MS(100), E_NETWORK_STARTUP);
            break;

        case ZPS_EVENT_ERROR:
            DBG_vPrintf(TRACE_JOIN, "ZPS ERR: 0x%x\r\n", sStackEvent.uEvent.sAfErrorEvent.eError);
            vStartStopTimer(APP_JoinTimer, APP_TIME_MS(100), E_NETWORK_STARTUP);
            break;

        default:
            vUnhandledEvent(g_sDevice.eState, sStackEvent.eType);
            break;
        }

    } else
    {
        DBG_vPrintf(TRACE_JOIN, "Discovery, no event\r\n");

    }
}


/****************************************************************************
 *
 * NAME: vHandleNetworkJoinEvent
 *
 * DESCRIPTION:
 * Handles stack events when the router is in its network join state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkJoinEvent(ZPS_tsAfEvent sStackEvent)
{
    switch (sStackEvent.eType)
    {
    case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
        vJoinedNetwork(sStackEvent);
        break;

    case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
        vJoinedNetwork(sStackEvent);
        break;

    case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        DBG_vPrintf(TRACE_JOIN, "Join Failed: 0x%x\r\n", sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status);
        u8DiscovedNWKJoinCount++;
        vJoinStoredNWK();
        break;

    case ZPS_EVENT_ERROR:

        DBG_vPrintf(TRACE_JOIN, "ZPS ERR: 0x%x\r\n", sStackEvent.uEvent.sAfErrorEvent.eError);

        vStartStopTimer(APP_JoinTimer, APP_TIME_MS(1000), E_NETWORK_STARTUP);

        break;

    default:
        vUnhandledEvent(g_sDevice.eState, sStackEvent.eType);
        break;
    }
}

/****************************************************************************
 *
 * NAME: vJoinedNetwork
 *
 * DESCRIPTION:
 * Sets up the node once is has joined the NWK
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vJoinedNetwork(ZPS_tsAfEvent sStackEvent)
{
    ZPS_tsNwkNib *thisNib;

    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    /* Store EPID for future rejoin */
    ZPS_eAplAibSetApsUseExtendedPanId(ZPS_u64NwkNibGetEpid(ZPS_pvAplZdoGetNwkHandle()));

    ZPS_bNwkNibAddrMapAddEntry(ZPS_pvAplZdoGetNwkHandle(), 0x0000, ZPS_psAplAibGetAib()->u64ApsTrustCenterAddress);

    DBG_vPrintf(TRACE_JOIN, "\r\nJoined Addr: 0x%x, CH: %d, PAN: %x, TrustC: %016llx\r\n",
                sStackEvent.uEvent.sNwkJoinedEvent.u16Addr, thisNib->sPersist.u8VsChannel,
                thisNib->sPersist.u16VsPanId, ZPS_psAplAibGetAib()->u64ApsTrustCenterAddress);

    vStartStopTimer(APP_JoinTimer, APP_TIME_MS(100), E_NETWORK_RUN);

    bRejoining = FALSE;
    g_sDevice.nwDesc = tsDiscovedNWKList[u8DiscovedNWKJoinCount];

    PDM_vSaveRecord(&g_sDevicePDDesc);

    ZPS_eAplZdoPermitJoining(0xff);

}

/****************************************************************************
 *
 * NAME: vJoinStoredNWK
 *
 * DESCRIPTION:
 * Handles stack events when the Controller is in its rescan state
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vJoinStoredNWK(void)
{
    ZPS_teStatus eStatus = 0xFF;

    DBG_vPrintf(TRACE_JOIN, "Join Count: %d,NWKs Found: %d\r\n", u8DiscovedNWKJoinCount, u8DiscovedNWKListCount);

    while (u8DiscovedNWKJoinCount < u8DiscovedNWKListCount)
    {
        DBG_vPrintf(TRACE_JOIN, "Joining PAN: %016llx, Index: %d\r\n", tsDiscovedNWKList[u8DiscovedNWKJoinCount].u64ExtPanId, u8DiscovedNWKJoinCount);
        eStatus = ZPS_eAplZdoJoinNetwork(&tsDiscovedNWKList[u8DiscovedNWKJoinCount]);

        if (ZPS_E_SUCCESS == eStatus)
        {
            g_sDevice.eState = E_NETWORK_JOINING;
            break;
        } else
        {
            DBG_vPrintf(TRACE_JOIN, "Join ERR: %x\r\n", eStatus);
            u8DiscovedNWKJoinCount++;
        }
    }

    if (u8DiscovedNWKJoinCount >= u8DiscovedNWKListCount)
    {
        DBG_vPrintf(TRACE_JOIN, "Cannot join found networks, rescan...\r\n");
        u8DiscovedNWKJoinCount = 0;
        vStartStopTimer(APP_JoinTimer, APP_TIME_MS(1000), E_NETWORK_STARTUP);
    }
}

/****************************************************************************
 *
 * NAME: vSaveDiscoveredNWKS
 *
 * DESCRIPTION:
 * Stores the list of NWKS from a scan
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vSaveDiscoveredNWKS(ZPS_tsAfEvent sStackEvent)
{
    uint8 i;

    u8DiscovedNWKJoinCount = 0;
    u8DiscovedNWKListCount = 0;
    memset(&tsDiscovedNWKList, 0, ((sizeof(ZPS_tsNwkNetworkDescr)) * MAX_SINGLE_CHANNEL_NETWORKS));

    for (i = 0; i < sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount && i < MAX_SINGLE_CHANNEL_NETWORKS; i++)
    {
        if (sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8PermitJoining &&
#if defined(TARGET_END)
            sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8EndDeviceCapacity
#else
            sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8RouterCapacity
#endif
            )
        {
            tsDiscovedNWKList[u8DiscovedNWKListCount] = sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i];
            u8DiscovedNWKListCount++;
        }
    }
}


/****************************************************************************
 *
 * NAME: vRestoreLastNWK
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PUBLIC void vRestoreLastNWK(ZPS_tsNwkNetworkDescr *desc)
{
    memcpy(&tsDiscovedNWKList[0], desc, (sizeof(ZPS_tsNwkNetworkDescr)));
    u8DiscovedNWKJoinCount = 1;
    u8DiscovedNWKListCount = 1;
}

/****************************************************************************
 *
 * NAME: vDisplayDiscoveredNWKS
 *
 * DESCRIPTION:
 * Lists the discovered NWKs to the UART.
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vDisplayDiscoveredNWKS(void)
{
    uint8 i;

    DBG_vPrintf(TRACE_JOIN, "Discovered networks:\r\n--------\r\n");

    if (u8DiscovedNWKListCount > 0)
    {
        for (i = 0; i < u8DiscovedNWKListCount; i++)
        {
            DBG_vPrintf(TRACE_JOIN, "Index: %d PAN: %016llx, Permit J: %d, CH: %d\r\n", i,
                        tsDiscovedNWKList[i].u64ExtPanId,
                        tsDiscovedNWKList[i].u8PermitJoining,
                        tsDiscovedNWKList[i].u8LogicalChan
                        );
        }
    }
    else
        DBG_vPrintf(TRACE_JOIN, "None. Please retry with AT CMD 'ATRS'.\r\n");

    DBG_vPrintf(TRACE_JOIN, "--------\r\n");
}

/****************************************************************************
 * NAME: AT_listNetworkScaned
 *
 * DESCRIPTION:
 * at command action function: list network information scanned
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   useless
 *
 * RETURNS:
 * void
 ****************************************************************************/
int AT_listNetworkScaned(uint16 *regAddr)
{

    uint8 i;
    int j;

    uart_printf("Discovered networks:\r\n");

    char tmp[128];
    if (u8DiscovedNWKListCount > 0)
    {
        for (i = 0; i < u8DiscovedNWKListCount; i++)
        {
            j = sprintf(tmp, "Index: %d, PAN: %08x%08x, Permit J: %d, CH: %d\r\n", i,
                        (uint32)(tsDiscovedNWKList[i].u64ExtPanId >> 32),
                        (uint32)(tsDiscovedNWKList[i].u64ExtPanId),
                        tsDiscovedNWKList[i].u8PermitJoining,
                        tsDiscovedNWKList[i].u8LogicalChan
                        );
            uart_tx_data(tmp, j);
        }
    }
    else
    {
        uart_printf("None. Please retry with AT CMD 'ATRS'.\r\n");
    }
    uart_printf("--------\r\n");
    return OK;
}

/****************************************************************************
*
* NAME: API_listNetworkScaned
*
* DESCRIPTION:
*
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
* int
* apiSpec, returned tsApiSpec Frame
*
****************************************************************************/
int API_listNetworkScaned_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
    int i = 0;
    tsNwkInfo nwkInfo;

	/* Response the num-1 packet through callback function, then return 2~n directly */

	for (i = 0; i < u8DiscovedNWKListCount; i++)
	{
		memset(&nwkInfo, 0, sizeof(tsNwkInfo));
		nwkInfo.index = i;
		nwkInfo.isPermitJoin = tsDiscovedNWKList[i].u8PermitJoining;
		nwkInfo.radioChannel = tsDiscovedNWKList[i].u8LogicalChan;
		nwkInfo.panId0 = (uint32)(tsDiscovedNWKList[i].u64ExtPanId);
		nwkInfo.panId1 = (uint32)(tsDiscovedNWKList[i].u64ExtPanId >> 32);
		/* return num-1 */
		if(1 == u8DiscovedNWKListCount)
		{
			  if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
			  {
					tsLocalAtResp localAtResp;
					memset(&localAtResp, 0, sizeof(tsLocalAtResp));

					/* Assemble LocalAtResp */
					assembleLocalAtResp(&localAtResp,
										reqApiSpec->payload.localAtReq.frameId,
										ATLA,
										AT_OK,
										(uint8*)&nwkInfo,
										sizeof(tsNwkInfo));

					/* Assemble apiSpec */
					assembleApiSpec(respApiSpec,
									API_LOCAL_AT_RESP,
									(uint8*)&localAtResp,
									sizeof(tsLocalAtResp));
			  }
			  else if(API_REMOTE_AT_REQ == reqApiSpec->teApiIdentifier)
			  {
				    tsRemoteAtResp remoteAtResp;
					memset(&remoteAtResp, 0, sizeof(tsRemoteAtResp));

					/* Assemble RemoteAtResp */
					assembleRemoteAtResp(&remoteAtResp,
										 reqApiSpec->payload.remoteAtReq.frameId,
										 ATLA,
										 AT_OK,
										 (uint8*)&nwkInfo,
										 sizeof(tsNwkInfo),
										 reqApiSpec->payload.remoteAtReq.unicastAddr);

					/* Assemble apiSpec */
					assembleApiSpec(respApiSpec,
									API_REMOTE_AT_RESP,
									(uint8*)&remoteAtResp,
									sizeof(tsRemoteAtResp));
			  }
		}
		else
		{
			uint8 tmp[sizeof(tsApiSpec)];
			tsApiSpec directApiSpec;
			memset(&directApiSpec, 0, sizeof(directApiSpec));

			  if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
			  {
					tsLocalAtResp localAtResp;
					memset(&localAtResp, 0, sizeof(tsLocalAtResp));

					/* Assemble LocalAtResp */
					assembleLocalAtResp(&localAtResp,
										reqApiSpec->payload.localAtReq.frameId,
										ATLA,
										AT_OK,
										(uint8*)&nwkInfo,
										sizeof(tsNwkInfo));

					/* Assemble apiSpec */
					assembleApiSpec(&directApiSpec,
									API_LOCAL_AT_RESP,
									(uint8*)&localAtResp,
									sizeof(tsLocalAtResp));
                    /* UART ACK */
					CMI_vUrtAckDistributor(&directApiSpec);
			  }
			  else if(API_REMOTE_AT_REQ == reqApiSpec->teApiIdentifier)
			  {
					tsRemoteAtResp remoteAtResp;
					memset(&remoteAtResp, 0, sizeof(tsRemoteAtResp));

					/* Assemble RemoteAtResp */
					assembleRemoteAtResp(&remoteAtResp,
										 reqApiSpec->payload.remoteAtReq.frameId,
										 ATLA,
										 AT_OK,
										 (uint8*)&nwkInfo,
										 sizeof(tsNwkInfo),
										 reqApiSpec->payload.remoteAtReq.unicastAddr);

					/* Assemble apiSpec */
					assembleApiSpec(&directApiSpec,
									API_REMOTE_AT_RESP,
									(uint8*)&remoteAtResp,
									sizeof(tsRemoteAtResp));
					 /* ACK unicast to u16SrcAddr */
					 int size = i32CopyApiSpec(&directApiSpec, tmp);
					 API_bSendToAirPort(UNICAST, reqApiSpec->payload.remoteAtReq.unicastAddr, tmp, size);
			  }
		}
	}
    return OK;
}
/****************************************************************************
 *
 * NAME: vUnhandledEvent
 *
 * DESCRIPTION:
 * Displays the unhandled state and event
 *
 * PARAMETERS: Name         RW  Usage
 *             sAppEvent  R   Contains details of the stack event
 *
 * RETURNS:
 * Sequence number
 *
 ****************************************************************************/
PUBLIC void vUnhandledEvent(uint8 eState, ZPS_teAfEventType eType)
{
    DBG_vPrintf(TRACE_JOIN, "Unhandled event: state: %d, Event: 0x%02x\r\n", eState, eType);
}



/****************************************************************************
 *
 * NAME: vStartStopTimer
 *
 * DESCRIPTION:
 * Stops and starts the given timer
 *
 * PARAMETERS:
 * hSWTimer      handle of timer
 * u32Ticks        Ticks for timer start
 * eNextState     state to move to after timer start
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartStopTimer(OS_thSWTimer hSWTimer, uint32 u32Ticks, teState eNextState)
{
    if (OS_eGetSWTimerStatus(hSWTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(hSWTimer);
    }
    OS_eStartSWTimer(hSWTimer, u32Ticks, NULL);
    g_sDevice.eState = eNextState;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
