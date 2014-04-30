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

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void handleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent);
void vHandleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent);
extern bool sendToAir(uint16 txmode, uint16 unicastDest, tsApiFrame *apiFrame, teFrameType type, uint8 *buff, int len);

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
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskMyEndPoint)
{
    ZPS_tsAfEvent sStackEvent;

    DBG_vPrintf(TRACE_EP, "-EndPoint- \r\n");

    if (g_sDevice.eState != E_NETWORK_RUN)
        return;

    if (OS_eCollectMessage(APP_msgMyEndPointEvents, &sStackEvent) == OS_E_OK)
    {
        if ((ZPS_EVENT_APS_DATA_INDICATION == sStackEvent.eType))
        {
            DBG_vPrintf(TRACE_EP, "[D_IND] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);

            /* Handle stack event's data */
            vHandleDataIndicatorEvent(sStackEvent);
        }
        else if (ZPS_EVENT_APS_DATA_CONFIRM == sStackEvent.eType)
        {
            if (g_sDevice.config.txMode == BROADCAST)
            {
                DBG_vPrintf(TRACE_EP, "[D_CFM] from 0x%04x \r\n",
                            sStackEvent.uEvent.sApsDataConfirmEvent.uDstAddr.u16Addr);
            }
        }
        else if (ZPS_EVENT_APS_DATA_ACK == sStackEvent.eType)
        {
            DBG_vPrintf(TRACE_EP, "[D_ACK] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataAckEvent.u16DstAddr);
        }
        else
        {
            DBG_vPrintf(TRACE_EP, "[UNKNOWN] event: 0x%x\r\n", sStackEvent.eType);
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
void vHandleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent)
{
    /* Call API support layer */
    int ret = API_i32AdsStackEventProc(&sStackEvent);
    if(!ret)
    {
    	/* report module error */
    	DBG_vPrintf(TRACE_EP, "APS: process stack event fail.\r\n");
    }
}

/****************************************************************************
 *
 * NAME: handleDataIndicatorEvent
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
void handleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent)
{
    PDUM_thAPduInstance hapdu_ins;
    uint16 u16PayloadSize;
    uint16 u16FreeSpace;
    uint8 *payload_addr;
    uint8 lqi;
    uint16 pwmWidth;

    hapdu_ins = sStackEvent.uEvent.sApsDataIndEvent.hAPduInst;
    lqi = sStackEvent.uEvent.sApsDataIndEvent.u8LinkQuality;
    u16PayloadSize = PDUM_u16APduInstanceGetPayloadSize(hapdu_ins);
    payload_addr = PDUM_pvAPduInstanceGetPayload(hapdu_ins);

    DBG_vPrintf(TRACE_EP, "lqi: %d \r\n", lqi);
    pwmWidth = lqi * 500 / 110;
    if (pwmWidth > 500)
        pwmWidth = 500;

    vAHI_TimerStartRepeat(E_AHI_TIMER_1, 500 - pwmWidth, 500 + pwmWidth);

    //we drop packets under AT mode
    //if (g_sDevice.eMode == E_MODE_AT)
    //{
    //    PDUM_eAPduFreeAPduInstance(hapdu_ins);
    //    return;
    //}

    //de-assemble frame
    bool validFrame = FALSE;
    tsApiFrame apiFrame;
    deassembleApiFrame(payload_addr, u16PayloadSize, &apiFrame, &validFrame);

    if (!validFrame)
    {
        DBG_vPrintf(TRACE_EP, "Not a valid frame, drop it.\r\n");
        PDUM_eAPduFreeAPduInstance(hapdu_ins);
        return;
    }

    uint16 u16SrcAddr = sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;

    //analyze the frame, to see wether a control frame etc.
    switch (apiFrame.frameType)
    {
    /* Control frame */
    case FRM_CTRL:
        break;
    /* Query register/on-chip value frame */
    case FRM_QUERY:

        break;
    /* Query response frame */
    case FRM_QUERY_RESP:

    	break;
    /* Data frame */
    case FRM_DATA:
        {
            OS_eEnterCriticalSection(mutexTxRb);
            u16FreeSpace = ringbuffer_free_space(&rb_tx_uart);
            OS_eExitCriticalSection(mutexTxRb);

            DBG_vPrintf(TRACE_EP, "Payload: %d, txfree: %d \r\n", apiFrame.payloadLen, u16FreeSpace);

            if (apiFrame.payloadLen > u16FreeSpace)
            {
                OS_eActivateTask(APP_taskMyEndPoint); //read later
            }
            else
            {
                uart_tx_data(apiFrame.payload.data, apiFrame.payloadLen);
                PDUM_eAPduFreeAPduInstance(hapdu_ins);
            }
            break;
        }
#ifdef OTA_SERVER
    case FRM_OTA_REQ:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);

            uint16 cltAddr = u16SrcAddr;
            uint32 blkIdx  = apiFrame.payload.frmOtaReq.blockIdx;
            if (blkIdx >= g_sDevice.otaTotalBlocks)
                break;

            uint8 buff[OTA_BLOCK_SIZE];
            uint16 rdLen = ((blkIdx + 1) * OTA_BLOCK_SIZE > g_sDevice.otaTotalBytes) ?
                           (g_sDevice.otaTotalBytes - blkIdx * OTA_BLOCK_SIZE) :
                           (OTA_BLOCK_SIZE);
            if (rdLen > OTA_BLOCK_SIZE)
                rdLen = OTA_BLOCK_SIZE;

            APP_vOtaFlashLockRead(blkIdx * OTA_BLOCK_SIZE, rdLen, buff);

            DBG_vPrintf(TRACE_EP, "OTA_REQ: blkIdx: %d \r\n", blkIdx);

            tsFrmOtaResp resp;
            resp.blockIdx = blkIdx;
            memcpy(&resp.block[0], buff, rdLen);
            resp.len = rdLen;
            resp.crc = g_sDevice.otaCrc;
            sendToAir(UNICAST, cltAddr, &apiFrame, FRM_OTA_RESP, (uint8 *)(&resp), sizeof(resp));
            break;
        }
    case FRM_OTA_ABT_RESP:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_ABT_RESP: from 0x%04x \r\n", u16SrcAddr);
            uart_printf("OTA: abort ack from 0x%04x.\r\n", u16SrcAddr);
            break;
        }
    case FRM_OTA_UPG_REQ:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_UPG_REQ: from 0x%04x \r\n", u16SrcAddr);
            uart_printf("OTA: Node 0x%04x's OTA download done, crc check ok.\r\n", u16SrcAddr);

            uint8 dummy = 0;
            sendToAir(UNICAST, u16SrcAddr, &apiFrame, FRM_OTA_UPG_RESP, (uint8 *)(&dummy), 1);

            break;
        }
    case FRM_OTA_ST_RESP:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_ST_RESP: from 0x%04x \r\n", u16SrcAddr);
            if (apiFrame.payload.frmOtaStResp.inOTA)
            {
                uart_printf("OTA: Node 0x%04x's OTA status: %d%%.\r\n", u16SrcAddr,
                            apiFrame.payload.frmOtaStResp.per);
            }
            else
                uart_printf("OTA: Node 0x%04x's is not in OTA or OTA finished.\r\n");

            break;
        }
#endif
#ifdef OTA_CLIENT
    case FRM_OTA_NTF:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            if (!g_sDevice.supportOTA)
                break;

            g_sDevice.otaReqPeriod  = apiFrame.payload.frmOtaNtf.reqPeriodMs;
            g_sDevice.otaTotalBytes = apiFrame.payload.frmOtaNtf.totalBytes;
            g_sDevice.otaSvrAddr16  = u16SrcAddr;
            g_sDevice.otaCurBlock   = 0;
            g_sDevice.otaTotalBlocks = (g_sDevice.otaTotalBytes % OTA_BLOCK_SIZE == 0) ?
                                       (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE) :
                                       (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE + 1);
            g_sDevice.otaDownloading = 1;
            DBG_vPrintf(TRACE_EP, "OTA_NTF: %d blks \r\n", g_sDevice.otaTotalBlocks);
            PDM_vSaveRecord(&g_sDevicePDDesc);

            //erase covered sectors
            APP_vOtaFlashLockEraseAll();

            //start the ota task
            OS_eActivateTask(APP_taskOTAReq);
            break;
        }

    case FRM_OTA_RESP:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            uint32 blkIdx = apiFrame.payload.frmOtaResp.blockIdx;
            uint32 offset = blkIdx * OTA_BLOCK_SIZE;
            uint16 len    = apiFrame.payload.frmOtaResp.len;
            uint32 crc    = apiFrame.payload.frmOtaResp.crc;

            if (blkIdx == g_sDevice.otaCurBlock)
            {
                DBG_vPrintf(TRACE_EP, "OTA_RESP: Blk: %d\r\n", blkIdx);
                APP_vOtaFlashLockWrite(offset, len, apiFrame.payload.frmOtaResp.block);
                g_sDevice.otaCurBlock += 1;
                g_sDevice.otaCrc = crc;
                if (g_sDevice.otaCurBlock % 100 == 0)
                    PDM_vSaveRecord(&g_sDevicePDDesc);
            }
            else
            {
                DBG_vPrintf(TRACE_EP, "OTA_RESP: DesireBlk: %d, RecvBlk: %d \r\n", g_sDevice.otaCurBlock, blkIdx);
            }

            if (g_sDevice.otaCurBlock >= g_sDevice.otaTotalBlocks)
            {
                clientOtaFinishing();
            }
            break;
        }
    case FRM_OTA_ABT_REQ:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_ABT_REQ: from 0x%04x \r\n", u16SrcAddr);
            if (g_sDevice.otaDownloading > 0)
            {
                g_sDevice.otaDownloading = 0;
                g_sDevice.otaCurBlock = 0;
                g_sDevice.otaTotalBytes = 0;
                g_sDevice.otaTotalBlocks = 0;
            }
            uint8 dummy = 0;
            sendToAir(UNICAST, u16SrcAddr, &apiFrame, FRM_OTA_ABT_RESP, (uint8 *)(&dummy), 1);

            break;
        }
    case FRM_OTA_ST_REQ:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_ST_REQ: from 0x%04x \r\n", u16SrcAddr);

            tsFrmOtaStatusResp resp;
            resp.inOTA = (g_sDevice.otaDownloading > 0);
            resp.per = 0;
            if (resp.inOTA && g_sDevice.otaTotalBlocks > 0)
            {
                resp.per = (uint8)((g_sDevice.otaCurBlock * 100) / g_sDevice.otaTotalBlocks);
            }
            sendToAir(UNICAST, u16SrcAddr, &apiFrame, FRM_OTA_ST_RESP,
                      (uint8 *)(&resp), sizeof(resp));

            break;
        }
    case FRM_OTA_UPG_RESP:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            DBG_vPrintf(TRACE_EP, "FRM_OTA_UPG_RESP: from 0x%04x \r\n", u16SrcAddr);

            g_sDevice.otaDownloading = 0;
            PDM_vSaveRecord(&g_sDevicePDDesc);

            APP_vOtaKillInternalReboot();
            break;
        }
#endif
    case FRM_TOPO_REQ:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);

            DBG_vPrintf(TRACE_EP, "FRM_TOPO_REQ: from 0x%04x \r\n", u16SrcAddr);

            tsFrmTOPOResp resp;
            resp.nodeMacAddr0 = (uint32)ZPS_u64AplZdoGetIeeeAddr();
            resp.nodeMacAddr1 = (uint32)(ZPS_u64AplZdoGetIeeeAddr() >> 32);
            resp.nodeFWVer    = (uint16)(FW_VERSION);
            sendToAir(UNICAST, u16SrcAddr, &apiFrame, FRM_TOPO_RESP, (uint8 *)(&resp), sizeof(resp));
            break;
        }
    case FRM_TOPO_RESP:
        {
            PDUM_eAPduFreeAPduInstance(hapdu_ins);
            uint32 mac0 = (uint32)apiFrame.payload.frmTopoResp.nodeMacAddr0;
            uint32 mac1 = (uint32)apiFrame.payload.frmTopoResp.nodeMacAddr1;
            uint16 fwver = (uint16)apiFrame.payload.frmTopoResp.nodeFWVer;
            int8 dbm = (lqi - 305) / 3;

            DBG_vPrintf(TRACE_EP, "FRM_TOPO_RESP: from 0x%04x \r\n", u16SrcAddr);

            uart_printf("+--Node resp--\r\n|--0x%04x,%08lx%08lx,LQI:%d,DBm:%d,Ver:0x%04x\r\n", u16SrcAddr,
                        mac1, mac0, lqi, dbm, fwver);
            break;
        }
    }

}
