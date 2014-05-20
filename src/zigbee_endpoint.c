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

	uint8 tmp[sizeof(tsApiSpec)] = {0};
	tsApiSpec apiSpec;
	memset(&apiSpec, 0, sizeof(tsApiSpec));

	if(1 == g_sDevice.otaDownloading)
	{
        tsOtaReq otaReq;
        memset(&otaReq, 0, sizeof(tsOtaReq));

        otaReq.blockIdx = g_sDevice.otaCurBlock;

        DBG_vPrintf(TRUE, "-OTAReq-\r\nreq blk: %d / %dms\r\n",
        		    otaReq.blockIdx, g_sDevice.otaReqPeriod);

        /* Coordinator can not act as a OTA client */
        if (ZPS_u16AplZdoGetNwkAddr() == 0x0)
        {
            DBG_vPrintf(TRUE, "Invalid ota client addr: 0x0000\r\n");
            g_sDevice.otaDownloading = 0;
        }

        /* package apiSpec */
	    apiSpec.startDelimiter = API_START_DELIMITER;
	    apiSpec.length = sizeof(tsOtaReq);
	    apiSpec.teApiIdentifier = API_OTA_REQ;
	    apiSpec.payload.otaReq = otaReq;
	    apiSpec.checkSum = calCheckSum((uint8*)&otaReq, apiSpec.length);

	   /* send through AirPort */
	   int size = i32CopyApiSpec(&apiSpec, tmp);
	   API_bSendToAirPort(UNICAST, g_sDevice.otaSvrAddr16, tmp, size);

	   /* Require per otaReqPeriod */
	   vResetATimer(APP_OTAReqTimer, APP_TIME_MS(g_sDevice.otaReqPeriod));
	}
	else if(2 == g_sDevice.otaDownloading)
	{
		/* package apiSpec */
		apiSpec.startDelimiter = API_START_DELIMITER;
		apiSpec.length = 1;
		apiSpec.teApiIdentifier = API_OTA_UPG_REQ;
		apiSpec.payload.dummyByte = 0;
		apiSpec.checkSum = 0;

		/* send through AirPort */
		int size = i32CopyApiSpec(&apiSpec, tmp);
		API_bSendToAirPort(UNICAST, g_sDevice.otaSvrAddr16, tmp, size);

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


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
