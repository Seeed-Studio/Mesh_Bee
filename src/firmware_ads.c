/*
 * firmware_ads.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao & Oliver Wang
 * Create Time: 2014/04
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
#include "firmware_ads.h"
#include "common.h"
#include "zps_apl_aib.h"
#include "firmware_at_api.h"

#ifndef TRACE_ADS
#define TRACE_ADS  FALSE
#endif
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void ADS_vHandleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent);

/****************************************************************************/
/***        Tasks                                                          ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_taskMyEndPoint
 *
 * DESCRIPTION:
 * Task to handle to end point(1) events (transmit-related event)
 * (AirPort Data Server)ADS module
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskMyEndPoint)
{
    ZPS_tsAfEvent sStackEvent;

    DBG_vPrintf(TRACE_ADS, "-EndPoint- \r\n");

    if (g_sDevice.eState != E_NETWORK_RUN)
        return;

    if (OS_eCollectMessage(APP_msgMyEndPointEvents, &sStackEvent) == OS_E_OK)
    {
        if ((ZPS_EVENT_APS_DATA_INDICATION == sStackEvent.eType))
        {
            DBG_vPrintf(TRACE_ADS, "[D_IND] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);

            /* Handle stack event's data from AirPort */
            ADS_vHandleDataIndicatorEvent(sStackEvent);
        }
        else if (ZPS_EVENT_APS_DATA_CONFIRM == sStackEvent.eType)
        {
            if (g_sDevice.config.txMode == BROADCAST)
            {
                DBG_vPrintf(TRACE_ADS, "[D_CFM] from 0x%04x \r\n",
                            sStackEvent.uEvent.sApsDataConfirmEvent.uDstAddr.u16Addr);
            }
        }
        else if (ZPS_EVENT_APS_DATA_ACK == sStackEvent.eType)
        {
            DBG_vPrintf(TRACE_ADS, "[D_ACK] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataAckEvent.u16DstAddr);
        }
        else
        {
            DBG_vPrintf(TRACE_ADS, "[UNKNOWN] event: 0x%x\r\n", sStackEvent.eType);
        }
    }
}


/****************************************************************************
 *
 * NAME: vHandleDataIndicatorEvent
 *
 * DESCRIPTION:
 * handle endpoint data indicator event
 *
 * PARAMETERS: Name         RW  Usage
 *             sStackEvent  R
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void ADS_vHandleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent)
{
	/* if AirPort receive a event, sleep later */
#ifdef TARGET_END
	if(E_MODE_API == g_sDevice.eMode || E_MODE_DATA == g_sDevice.eMode)
	{
	    vSleepSchedule();
	}
#endif
    /* Call API support layer */
    int ret = API_i32AdsStackEventProc(&sStackEvent);
    if(ret!=OK)
    {
    	/* report module error */
    	DBG_vPrintf(TRACE_ADS, "ADS: process stack event fail.\r\n");
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
