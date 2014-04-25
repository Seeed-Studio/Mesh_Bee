/*
 * zigbee_endpoint.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log : Oliver Wang Modify 2014/03
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
#include "stdlib.h"
#include "common.h"
#include "firmware_uart.h"
#include "zigbee_endpoint.h"
#include "zigbee_node.h"
#include "firmware_at_api.h"
#include "firmware_ota.h"
#include "firmware_hal.h"
#include "firmware_api_pack.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_EP
#define TRACE_EP    TRUE
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
PRIVATE bool   bActiveByTimer = FALSE;


/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern tsDevice g_sDevice;
extern PDM_tsRecordDescriptor g_sDevicePDDesc;


/****************************************************************************
 *
 * NAME: APP_taskOTAReq
 *
 * DESCRIPTION:
 * send OTA request
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskOTAReq)
{
#ifdef OTA_CLIENT
    if (g_sDevice.otaDownloading < 1 || g_sDevice.eState <= E_NETWORK_JOINING)
        return;

    if (g_sDevice.otaDownloading == 1)
    {
        tsApiFrame frm;
        tsFrmOtaReq req;

        req.blockIdx = g_sDevice.otaCurBlock;

        DBG_vPrintf(TRACE_EP, "-OTAReq-\r\nreq blk: %d / %dms\r\n",
                    req.blockIdx, g_sDevice.otaReqPeriod);

        if (ZPS_u16AplZdoGetNwkAddr() == 0x0)
        {
            DBG_vPrintf(TRACE_EP, "Invalid ota client addr: 0x0000\r\n");
            g_sDevice.otaDownloading = 0;
        }

        sendToAir(UNICAST, g_sDevice.otaSvrAddr16, &frm, FRM_OTA_REQ, (uint8 *)(&req), sizeof(req));

        vResetATimer(APP_OTAReqTimer, APP_TIME_MS(g_sDevice.otaReqPeriod));
    }
    else if (g_sDevice.otaDownloading == 2)
    {
        tsApiFrame frm;
        uint8 dummy = 0;

        sendToAir(UNICAST, g_sDevice.otaSvrAddr16, &frm, FRM_OTA_UPG_REQ, (&dummy), 1);

        vResetATimer(APP_OTAReqTimer, APP_TIME_MS(1000));
    }
#endif
}


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: vResetATimer
 *
 * DESCRIPTION:
 * tool function for stop and then start a software timer
 *
 * PARAMETERS: Name         RW  Usage
 *             hSWTimer     R   handler to sw timer
 *             u32Ticks     R   ticks of cycle
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vResetATimer(OS_thSWTimer hSWTimer, uint32 u32Ticks)
{
    if (OS_eGetSWTimerStatus(hSWTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(hSWTimer);
    }
    OS_eStartSWTimer(hSWTimer, u32Ticks, NULL);
}



/****************************************************************************/
/***        Local Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: clientOtaRestartDownload
 *
 * DESCRIPTION:
 * restart OTA download when crc check failed
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void clientOtaRestartDownload()
{
    if (g_sDevice.otaTotalBytes == 0)
    {
        DBG_vPrintf(TRACE_EP, "otaTotalBytes info lost, cant restart download. \r\n");
        return;
    }
    // restart the downloading
    g_sDevice.otaCurBlock   = 0;
    g_sDevice.otaDownloading = 1;
    DBG_vPrintf(TRACE_EP, "restart downloading... \r\n");
    PDM_vSaveRecord(&g_sDevicePDDesc);

    //erase covered sectors
    APP_vOtaFlashLockEraseAll();

    //start the ota task
    OS_eActivateTask(APP_taskOTAReq);
}


/****************************************************************************
 *
 * NAME: clientOtaFinishing
 *
 * DESCRIPTION:
 * OTA download finishing routine, check the crc of total downloaded image
 * at the external flash
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void clientOtaFinishing()
{
#ifdef OTA_CLIENT
    DBG_vPrintf(TRACE_EP, "OtaFinishing: get all %d blocks \r\n", g_sDevice.otaCurBlock);
    g_sDevice.otaDownloading = 0;
    PDM_vSaveRecord(&g_sDevicePDDesc);

    //verify the external flash image
    uint8 au8Values[OTA_MAGIC_NUM_LEN];
    uint32 u32TotalImage = 0;
    bool valid = true;


    //first, check external flash to detect image header
    APP_vOtaFlashLockRead(OTA_MAGIC_OFFSET, OTA_MAGIC_NUM_LEN, au8Values);

    if (memcmp(magicNum, au8Values, OTA_MAGIC_NUM_LEN) == 0)
    {
        DBG_vPrintf(TRACE_EP, "OtaFinishing: found image magic num. \r\n");

        //read the image length out
        APP_vOtaFlashLockRead(OTA_IMAGE_LEN_OFFSET, 4, (uint8 *)(&u32TotalImage));

        if (u32TotalImage != g_sDevice.otaTotalBytes)
        {
            DBG_vPrintf(TRACE_EP, "OtaFinishing: total length not match. \r\n");
            valid = false;
        }
    }
    else
    {
        DBG_vPrintf(TRACE_EP, "OtaFinishing: not find magic num. \r\n");
        valid = false;
    }

    //second, check crc
    uint32 crc = imageCrc(u32TotalImage);
    DBG_vPrintf(TRACE_EP, "OtaFinishing: verify crc: 0x%x \r\n", crc);
    if (crc != g_sDevice.otaCrc)
    {
        DBG_vPrintf(TRACE_EP, "OtaFinishing: crc not match \r\n");
        valid = false;
    }

    if (valid)
    {
        //send upgrade request to ota server
        g_sDevice.otaDownloading = 2;
        PDM_vSaveRecord(&g_sDevicePDDesc);
        //OS_eActivateTask(APP_taskOTAReq);
        vResetATimer(APP_OTAReqTimer, APP_TIME_MS(1000));
        //APP_vOtaKillInternalReboot();
    }
    else
    {
        clientOtaRestartDownload();
    }
#endif
}





/****************************************************************************
 *
 * NAME: sendToAir
 *
 * DESCRIPTION:
 * send data with radio by broadcasting or unicasting
 * Package is finished out of this,so,just send it.
 * PARAMETERS: Name         RW  Usage
 *             txmode       R   ENUM: BROADCAST ,UNICAST
 *             unicastDest  R   16bit short address
 *             apiFrame     RW  pointer to tsApiFrame
 *             type         R   ENUM: teFrameType
 *             buff         R   pointer to data to be sent
 *             len          R   data len
 *
 * RETURNS:
 * TRUE: send ok
 * FALSE:failed to allocate APDU
 *
 ****************************************************************************/
bool sendToAir(uint16 txmode, uint16 unicastDest, tsApiFrame *apiFrame, teFrameType type, uint8 *buff, int len)
{
    PDUM_thAPduInstance hapdu_ins = PDUM_hAPduAllocateAPduInstance(apduZCL);

    if (hapdu_ins == PDUM_INVALID_HANDLE)
        return FALSE;

    //assemble the packet
    uint16 frameLen = assembleApiFrame(apiFrame, type, buff, len);
    //copy packet into apdu
    uint8 *payload_addr = PDUM_pvAPduInstanceGetPayload(hapdu_ins);
    copyApiFrame(apiFrame, payload_addr);
    PDUM_eAPduInstanceSetPayloadSize(hapdu_ins, frameLen);

    ZPS_teStatus st;
    if (txmode == BROADCAST)
    {
        DBG_vPrintf(TRACE_EP, "Broadcast %d ...\r\n", len);

        // apdu will be released by the stack automatically after the apdu is send
        st = ZPS_eAplAfBroadcastDataReq(hapdu_ins, TRANS_CLUSTER_ID,
                                        TRANS_ENDPOINT_ID, TRANS_ENDPOINT_ID,
                                        ZPS_E_BROADCAST_ALL, SEC_MODE_FOR_DATA_ON_AIR,
                                        0, NULL);

    }
    else if (txmode == UNICAST)
    {
        DBG_vPrintf(TRACE_EP, "Unicast %d ...\r\n", len);

        st = ZPS_eAplAfUnicastDataReq(hapdu_ins, TRANS_CLUSTER_ID,
                                         TRANS_ENDPOINT_ID, TRANS_ENDPOINT_ID,
                                         unicastDest, SEC_MODE_FOR_DATA_ON_AIR,
                                         0, NULL);
    }

    if (st != ZPS_E_SUCCESS)
    {
        /*
          we dont care about the failure anymore, because handling this failure will delay or
          even block the following waiting data. So just let it go and focus on the next data.
        */
        DBG_vPrintf(TRACE_EP, "Send failed: 0x%x, drop it... \r\n", st);
        PDUM_eAPduFreeAPduInstance(hapdu_ins);
        return FALSE;
    }
    return TRUE;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
