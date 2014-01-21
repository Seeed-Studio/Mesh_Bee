/*    
 * zigbee_zbp_utilities.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module 
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

/* Stack Includes */
#include <jendefs.h>
#include <string.h>
#include "app_timer_driver.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_nwk_sap.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "os_gen.h"
#include "dbg.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_TIMERS
#define TRACE_TIMERS    TRUE
#endif

#ifndef TRACE_ZBP_UTILS
#define TRACE_ZBP_UTILS    TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Tasks                                                          ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vDisplayTableSizes
 *
 * DESCRIPTION:
 * Displays the sizes of the various tables within the PRO stack
 *
 * PARAMETERS: None
 *
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC void vDisplayTableSizes(void)
{
    ZPS_tsNwkNib * thisNib;

    void * thisNet = ZPS_pvAplZdoGetNwkHandle();
    thisNib = ZPS_psNwkNibGetHandle(thisNet);

    DBG_vPrintf(TRACE_ZBP_UTILS, "Address Map:Size: %d: Record %d: %d: Total: %d ",
                                thisNib->sTblSize.u16AddrMap,
                                10,
                                (thisNib->sTblSize.u16AddrMap * 10)
                                );


    DBG_vPrintf(TRACE_ZBP_UTILS, "NT:Size: %d: Record %d: %d: Total: %d ",
                                  thisNib->sTblSize.u16NtActv,
                                  sizeof(ZPS_tsNwkActvNtEntry),
                                  (thisNib->sTblSize.u16NtActv * sizeof(ZPS_tsNwkActvNtEntry))
                                  );



    DBG_vPrintf(TRACE_ZBP_UTILS, "Routing Table:Size: %d: Record %d: %d: Total: %d ",
                                thisNib->sTblSize.u16Rt,
                                sizeof(ZPS_tsNwkRtEntry),
                                (thisNib->sTblSize.u16Rt * sizeof(ZPS_tsNwkRtEntry))
                                );


    DBG_vPrintf(TRACE_ZBP_UTILS, "Route Record:Size: %d: Record %d: %d: Total: %d ",
                                thisNib->sTblSize.u16Rct,
                                sizeof(ZPS_tsNwkRctEntry),
                                (thisNib->sTblSize.u16Rct * sizeof(ZPS_tsNwkRctEntry))
                                );
}


PUBLIC void vDisplayAddressMapTable(void)
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i = 0;

    DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nAddress Map Size: %d", thisNib->sTblSize.u16AddrMap);

    for( i=0;i<thisNib->sTblSize.u16AddrMap;i++)
    {
        DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nShort Addr: %04x, Ext Addr: %016llx,", thisNib->sTbl.pu16AddrMapNwk[i], thisNib->sTbl.pu64AddrMapExt[i]);
    }
}


PUBLIC void vDisplayNT( void )
{
ZPS_tsNwkNib * thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
uint8 i;

    DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nNT Size: %d\r\n", thisNib->sTblSize.u16NtActv);

    for( i = 0 ; i < thisNib->sTblSize.u16NtActv ; i++ )
    {
        DBG_vPrintf(TRACE_ZBP_UTILS, "Addr: 0x%04x, ExtAddr: 0x%016llx, LQI: %i, Failed TX's: %i, Auth: %i, %i %i %i %i %i %i, Active: %i, %i %i %i\r\n",
                    thisNib->sTbl.psNtActv[i].u16NwkAddr,
                    thisNib->sTbl.psNtActv[i].u64ExtAddr,
                    thisNib->sTbl.psNtActv[i].u8LinkQuality,
                    thisNib->sTbl.psNtActv[i].u8TxFailed,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1Authenticated,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1DeviceType,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1ExpectAnnc,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1LinkStatusDone,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1PowerSource,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1RxOnWhenIdle,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1SecurityMode,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1Used,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u3Age,
                    thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u3OutgoingCost
                    );
    }
}


/****************************************************************************
 *
 * NAME: vDisplayAPSTable
 *
 * DESCRIPTION:
 * Display the APS key table
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDisplayAPSTable(void)
{
    uint8 i;
    int8 j;

    ZPS_tsAplAib * tsAplAib;

    tsAplAib = ZPS_psAplAibGetAib();

    for ( i = 0 ; i < (tsAplAib->psAplDeviceKeyPairTable->u32SizeOfKeyDescriptorTable + 1) ; i++ )
    {
        DBG_vPrintf(TRACE_ZBP_UTILS, "MAC: %016llx \r\n", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u64DeviceAddress);
        DBG_vPrintf(TRACE_ZBP_UTILS, "KEY: " );

        for(j=0; j<16;j++)
        {
            DBG_vPrintf(TRACE_ZBP_UTILS, "%02x, ", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].au8LinkKey[j]);
        }
        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\n");
        DBG_vPrintf(TRACE_ZBP_UTILS, "Incoming FC: %d\r\n", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u32IncomingFrameCounter);
        DBG_vPrintf(TRACE_ZBP_UTILS, "Outgoing FC: %d\r\n", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u32OutgoingFrameCounter);
    }
}


/****************************************************************************
 *
 * NAME: vClearDiscNT
 *
 * DESCRIPTION:
 * Resets the discovery NT
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PUBLIC void vClearDiscNT(void)
{
    ZPS_tsNwkNib * thisNib;

    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    memset((uint8*)thisNib->sTbl.psNtDisc, 0, sizeof(ZPS_tsNwkDiscNtEntry) * thisNib->sTblSize.u8NtDisc);

}


/****************************************************************************
 *
 * NAME: vRemoveCoordParents
 *
 * DESCRIPTION:
 * Removes Coordinator parents from the scan results to test joining through
 * a router, and key establishment.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRemoveCoordParents(void)
{
    ZPS_tsNwkNib * thisNib;
    uint8 i;

    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    for( i = 0; i < thisNib->sTblSize.u8NtDisc; i++)
    {
        if(thisNib->sTbl.psNtDisc[i].u16NwkAddr == 0x0000 )
        {
            thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1JoinPermit = 0;
        }
    }

}


PUBLIC void vDisplayDiscNT(void)
{
    ZPS_tsNwkNib * thisNib;
    uint8 i;

    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    for( i = 0; i < thisNib->sTblSize.u8NtDisc; i++)
    {
        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nIndex: %d", i );

        DBG_vPrintf(TRACE_ZBP_UTILS, " EPID: %016llx", thisNib->sTbl.psNtDisc[i].u64ExtPanId);

        DBG_vPrintf(TRACE_ZBP_UTILS, " PAN: %04x", thisNib->sTbl.psNtDisc[i].u16PanId);

        DBG_vPrintf(TRACE_ZBP_UTILS, " SAddr: %04x", thisNib->sTbl.psNtDisc[i].u16NwkAddr);

        DBG_vPrintf(TRACE_ZBP_UTILS, " LQI %d\r\n", thisNib->sTbl.psNtDisc[i].u8LinkQuality);

        DBG_vPrintf(TRACE_ZBP_UTILS, " CH: %d", thisNib->sTbl.psNtDisc[i].u8LogicalChan);

        DBG_vPrintf(TRACE_ZBP_UTILS, " PJ: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1JoinPermit);

        DBG_vPrintf(TRACE_ZBP_UTILS, " Coord: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1PanCoord);

        DBG_vPrintf(TRACE_ZBP_UTILS, " RT Cap: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1ZrCapacity);

        DBG_vPrintf(TRACE_ZBP_UTILS, " ED Cap: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1ZedCapacity);

        DBG_vPrintf(TRACE_ZBP_UTILS, " Depth: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u4Depth);

        DBG_vPrintf(TRACE_ZBP_UTILS, " StPro: %d", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u4StackProfile);

        DBG_vPrintf(TRACE_ZBP_UTILS, " PP: %d\r\n", thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u1PotentialParent);
       }

}


/* Routing Table Functions */
PUBLIC bool bInRoutingTable( uint16 u16ShortAddress )
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i = 0;

    for( i=0;i<thisNib->sTblSize.u16Rt;i++)
    {
        if(thisNib->sTbl.psRt[i].u16NwkDstAddr == u16ShortAddress )
        {
            DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nGot Short Address: %02x", thisNib->sTbl.psRt[i].u16NwkDstAddr);
            return TRUE;
        }
    }

    return 0;
}


PUBLIC void vDisplayRoutingTable(void)
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i = 0;

    DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nRouting Table Size %d\r\n", thisNib->sTblSize.u16Rt);

    for( i=0;i<thisNib->sTblSize.u16Rt;i++)
    {
        DBG_vPrintf(TRACE_ZBP_UTILS,"Status: %d, Short Address: %02x, Next Hop: %02x\r\n",
                thisNib->sTbl.psRt[i].uAncAttrs.bfBitfields.u3Status,
                thisNib->sTbl.psRt[i].u16NwkDstAddr,
                thisNib->sTbl.psRt[i].u16NwkNxtHopAddr

                );
    }

}


PUBLIC void vClearRoutingTable(void)
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i = 0;

    DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nRouting Table Size %d", thisNib->sTblSize.u16Rt);

    for( i=0;i<thisNib->sTblSize.u16Rt;i++)
    {
        thisNib->sTbl.psRt[i].uAncAttrs.bfBitfields.u3Status = 3;
        thisNib->sTbl.psRt[i].u16NwkDstAddr = 0;
        thisNib->sTbl.psRt[i].u16NwkNxtHopAddr = 0;
    }
}


PUBLIC void vDisplayRouteRecordTable(void)
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i, j = 0;

    for( i=0;i<thisNib->sTblSize.u16Rct;i++)
    {
        DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nRelay Count: %i NwkdstAddr: 0x%04x", thisNib->sTbl.psRct[i].u8RelayCount, thisNib->sTbl.psRct[i].u16NwkDstAddr);
        for ( j = 0 ; j < ZPS_NWK_NIB_MAX_DEPTH_DEF * 2 ; j++)
        {
            DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nPath[%i]: %i", j, thisNib->sTbl.psRct[i].au16Path[j]);
        }
    }
}


PUBLIC void vClearRouteRecordTable(void)
{
    ZPS_tsNwkNib * thisNib;
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    uint8 i, j = 0;

    DBG_vPrintf(TRACE_ZBP_UTILS,"\r\nClearing Record Table");

    for( i=0;i<thisNib->sTblSize.u16Rct;i++)
    {
        thisNib->sTbl.psRct[i].u8RelayCount = 0;
        thisNib->sTbl.psRct[i].u16NwkDstAddr = 0xFFFE;
        for ( j = 0 ; j < ZPS_NWK_NIB_MAX_DEPTH_DEF * 2 ; j++)
        {
            thisNib->sTbl.psRct[i].au16Path[j] = 0;
        }
    }
}


void vDisplayNWKKey(void)
{
    uint8 i = 0;
    uint8 j = 0;

    ZPS_tsNwkNib * thisNib;

    void * thisNet = ZPS_pvAplZdoGetNwkHandle();
    thisNib = ZPS_psNwkNibGetHandle(thisNet);

    for(j=0;j<thisNib->sTblSize.u8SecMatSet;j++)
    {

        DBG_vPrintf(TRACE_ZBP_UTILS, "APP: Key");

        for(i = 0;i<16;i++)
        {
            DBG_vPrintf(TRACE_ZBP_UTILS, "%x", thisNib->sTbl.psSecMatSet[j].au8Key[i]);
        }

        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\n");
    }
}


PUBLIC void vDisplayNWKTransmitTable(void)
{
    vDisplayNT();
    vDisplayRoutingTable();
    vDisplayAddressMapTable();
    vDisplayAPSTable();
}


/****************************************************************************
*
* NAME: vDisplayBindingTable
*
* DESCRIPTION:
* Display the binding table to the UART
*
* PARAMETERS: None
*
*
* RETURNS:
* None
*
****************************************************************************/
PUBLIC void vDisplayBindingTable( void )
{
    uint32   j = 0;

    ZPS_tsAplAib * tsAplAib  = ZPS_psAplAibGetAib();

    DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nBind Size %d",  tsAplAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable[0].u32SizeOfBindingTable );

    for( j = 0 ; j < tsAplAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable[0].u32SizeOfBindingTable ; j++ )
    {
        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nMAC Dest %016llx", tsAplAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable[0].pvAplApsmeBindingTableEntryForSpSrcAddr[j].uDstAddress.u64Addr);
        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nEP Dest %d", tsAplAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable[0].pvAplApsmeBindingTableEntryForSpSrcAddr[j].u8DestinationEndPoint);
        DBG_vPrintf(TRACE_ZBP_UTILS, "\r\nCluster Dest %d", tsAplAib->psAplApsmeAibBindingTable->psAplApsmeBindingTable[0].pvAplApsmeBindingTableEntryForSpSrcAddr[j].u16ClusterId);
    }

}


#ifdef TRACE_TIMERS
/****************************************************************************
 *
 * NAME: vDebugTimers
 *
 * DESCRIPTION:
 * Prints out any timers that have expired/running for debug purposes.
 * Especially useful when determining if a timer is preventing the device
 * from entering sleep.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDebugTimers(void)
{
    /* Display any expired timers */
//  if (OS_E_SWTIMER_EXPIRED == OS_eGetSWTimerStatus(APP_RestartTimer))
//      DBG_vPrintf(TRACE_TIMERS, "APP_RestartTimer Expired\r\n");
//    if (OS_E_SWTIMER_EXPIRED == OS_eGetSWTimerStatus(APP_ButtonsScanTimer))
//        DBG_vPrintf(TRACE_TIMERS, "APP_ButtonsScanTimer Expired\r\n");
//    if (OS_E_SWTIMER_EXPIRED == OS_eGetSWTimerStatus(APP_ZclTimer))
//        DBG_vPrintf(TRACE_TIMERS, "APP_ZclTimer Expired\r\n");


    /* Display any running timers */
//  if (OS_E_SWTIMER_RUNNING == OS_eGetSWTimerStatus(APP_RestartTimer))
//      DBG_vPrintf(TRACE_TIMERS, "APP_RestartTimer Running\r\n");
//    if (OS_E_SWTIMER_RUNNING == OS_eGetSWTimerStatus(APP_ButtonsScanTimer))
//        DBG_vPrintf(TRACE_TIMERS, "APP_ButtonsScanTimer Running\r\n");
//    if (OS_E_SWTIMER_RUNNING == OS_eGetSWTimerStatus(APP_ZclTimer))
//        DBG_vPrintf(TRACE_TIMERS, "APP_ZclTimer Running\r\n");

}
#endif


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
