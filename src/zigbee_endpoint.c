/*    
 * zigbee_endpoint.c
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

#include "common.h"

#include "firmware_uart.h"
#include "zigbee_endpoint.h"
#include "zigbee_node.h"
#include "firmware_at_api.h"
#include "firmware_ota.h"

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
void handleDataIndicatorEvent(ZPS_tsAfEvent sStackEvent);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool   bActiveByTimer = FALSE;

struct ringbuffer rb_rx_uart;
struct ringbuffer rb_tx_uart;

uint8 rb_rx_mempool[RXFIFOLEN*3];
uint8 rb_tx_mempool[TXFIFOLEN*3];

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern tsDevice g_sDevice;
extern PDM_tsRecordDescriptor g_sDevicePDDesc;

/****************************************************************************/
/***        Tasks                                                          ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_taskMyEndPoint
 *
 * DESCRIPTION:
 * Task to handle to end point(1) events
 *
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

            handleDataIndicatorEvent(sStackEvent);
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
 * NAME: APP_taskHandleUartRx
 *
 * DESCRIPTION:
 * handle uart rx'ed data
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
static uint8 tmp[RXFIFOLEN];

OS_TASK(APP_taskHandleUartRx)
{
    uint32 dataCnt = 0;
    uint32 popCnt = 0;

    OS_eEnterCriticalSection(mutexRxRb);
    dataCnt = ringbuffer_data_size(&rb_rx_uart);
    OS_eExitCriticalSection(mutexRxRb);

    if (dataCnt == 0)  return;

    DBG_vPrintf(TRACE_EP, "-HandleUartRx- \r\n");
    //
    switch (g_sDevice.eMode)
    {
    case E_MODE_DATA:

        //read some data out
        popCnt = MIN(dataCnt, THRESHOLD_READ); 

        OS_eEnterCriticalSection(mutexRxRb);
        ringbuffer_pop(&rb_rx_uart, tmp, popCnt); 
        OS_eExitCriticalSection(mutexRxRb);
        
        if ((dataCnt - popCnt) >= THRESHOLD_READ) 
            OS_eActivateTask(APP_taskHandleUartRx); 
        else if ((dataCnt - popCnt) > 0) 
            vResetATimer(APP_tmrHandleUartRx, APP_TIME_MS(1)); 

        //AT filter to find AT delimiter
        if (searchAtStarter(tmp, popCnt)) 
        {
            g_sDevice.eMode = E_MODE_AT;
            PDM_vSaveRecord(&g_sDevicePDDesc); 
            uart_printf("Enter AT mode.\r\n");
            clear_ringbuffer(&rb_rx_uart);
        }
        //if not containing AT, send out the data
        else if (g_sDevice.eState == E_NETWORK_RUN) //if send, make sure network has been created.
        {
            tsApiFrame frm;
            sendToAir(g_sDevice.config.txMode, g_sDevice.config.unicastDstAddr,
                      &frm, FRM_DATA, tmp, popCnt); 
        }
        break;
    case E_MODE_AT:
        popCnt = MIN(dataCnt, RXFIFOLEN); 
        
        OS_eEnterCriticalSection(mutexRxRb);
        ringbuffer_read(&rb_rx_uart, tmp, popCnt); 
        OS_eExitCriticalSection(mutexRxRb); 
        
        int len = popCnt; 
        bool found = FALSE;
        while (len--)
        {
            if (tmp[len] == '\r' || tmp[len] == '\n') 
                found = TRUE;
        }
        
        if (!found && popCnt < RXFIFOLEN) 
        {
            //OS_eStartSWTimer(APP_tmrHandleUartRx, APP_TIME_MS(5), NULL); //handle after 1ms
            return;
        }

        int ret = processSerialCmd(tmp, popCnt); 

        char *resp;
        if (ret == OK)
            resp = "OK\r\n\r\n";
        if (ret == ERR)
            resp = "Error\r\n\r\n"; 
        if (ret == ERRNCMD)
            resp = "Error, invalid command\r\n\r\n"; 
        if (ret == OUTRNG)
            resp = "Error, out range\r\n\r\n"; 
        uart_printf(resp);
        
        OS_eEnterCriticalSection(mutexRxRb); 
        ringbuffer_pop(&rb_rx_uart, tmp, popCnt); 
        OS_eExitCriticalSection(mutexRxRb);
        break;
    case E_MODE_API:

        break;
    default:
        break;
    }

}

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
 * NAME: endpoint_vInitialize
 *
 * DESCRIPTION:
 * init endpoint(1)
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void endpoint_vInitialize()
{
    init_ringbuffer(&rb_rx_uart, rb_rx_mempool, RXFIFOLEN * 3);
    init_ringbuffer(&rb_tx_uart, rb_tx_mempool, TXFIFOLEN * 3);
}

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
    case FRM_CTRL:
        break;
    case FRM_QUERY:
        break;
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
            resp.nodeFWVer    = (uint16)(SW_VER);
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


/****************************************************************************
 *
 * NAME: sendToAir
 *
 * DESCRIPTION:
 * send data wich radio by broadcasting or unicasting
 *
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
        //we dont care about the failure anymore, because handling this failure will delay or
        //even block the following waiting data. So just let it go and focus on the next data.
        DBG_vPrintf(TRACE_EP, "Send failed: 0x%x, drop it... \r\n", st);
        PDUM_eAPduFreeAPduInstance(hapdu_ins);
        return FALSE;
    }
    return TRUE;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
