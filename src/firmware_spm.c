/*
 * firmware_uds.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Oliver Wang & Jack Shao
 * Create Time: 2014/04
 * Change Log : UDS and Stream Processing Machine(SPM)
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

#include "firmware_spm.h"
#include "firmware_cmi.h"
#include "firmware_at_api.h"
#include "firmware_uart.h"
#include "firmware_ringbuffer.h"
#include "firmware_api_pack.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/*
  data pool of SPM module,anything put into
  this pool will be processed by SPM
*/
struct ringbuffer rb_rx_spm;
uint8 spm_rx_mempool[SPM_RX_RB_LEN];

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void SPM_vProcStream(uint32 dataCnt);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
uint32 SPM_u32PullData(void *data, int len)
{
	uint32 free_cnt = 0;
	uint32 min_cnt = 0;
	uint32 avlb_cnt = 0;

	/* Cal how much free space do SPM have */
	OS_eEnterCriticalSection(mutexRxRb);
	free_cnt = ringbuffer_free_space(&rb_rx_spm);
	OS_eExitCriticalSection(mutexRxRb);

    min_cnt = MIN(free_cnt, len);
    DBG_vPrintf(TRACE_NODE, "rev_cnt: %u, free_cnt: %u \r\n", len, free_cnt);
    if(min_cnt > 0)
    {
    	OS_eEnterCriticalSection(mutexRxRb);
    	ringbuffer_push(&rb_rx_spm, data, min_cnt);
    	avlb_cnt = ringbuffer_data_size(&rb_rx_spm);
    	OS_eExitCriticalSection(mutexRxRb);
    }
    return avlb_cnt;
}



/****************************************************************************
 *
 * NAME: APP_taskHandleUartRx
 *
 * DESCRIPTION:
 * Stream Processing Machine(SPM), driving by software timer
 * Main state machine
 * State:  E_MODE_AT/E_MODE_DATA/E_MODE_API/E_MODE_MCU
 *
 * Note:
 *      In a Mesh topology network,if you use AT mode,there would be no way to
 * determine which sensor send what data,the data from different nodes would come
 * out of the base node's UART in a jumbled mess.so,API mode is the best choice.
 * All of the mode are format into ApiSpec frame,then processed by SPM
 * RETURNS:
 * void
 *
 ****************************************************************************/
static uint8 tmp[RXFIFOLEN];
OS_TASK(APP_taskHandleUartRx)
{
    uint32 dataCnt = 0;
    uint32 popCnt = 0;
    uint32 size = 0;
    tsApiSpec apiSpec;

    /* calculate data size of the ring buffer */
    OS_eEnterCriticalSection(mutexRxRb);
    dataCnt = ringbuffer_data_size(&rb_rx_spm);
    OS_eExitCriticalSection(mutexRxRb);

    /* if there is no data in SPM data pool, return */
    if (dataCnt == 0)  return;

    DBG_vPrintf(TRACE_EP, "-SPM running- \r\n");

    /* SPM State Machine */
    switch(g_sDevice.eMode)
    {
        /* AT mode */
        case E_MODE_AT:
        	 popCnt = MIN(dataCnt, RXFIFOLEN);
        	 OS_eEnterCriticalSection(mutexRxRb);
        	 ringbuffer_read(&rb_rx_spm, tmp, popCnt);
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
        		 return;
        	 }

        	 /* Process AT command */
        	 int ret = API_i32AtProcessSerialCmd(tmp, popCnt);

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

        	 /* Discard the treated part */
        	 OS_eEnterCriticalSection(mutexRxRb);
        	 ringbuffer_pop(&rb_rx_spm, tmp, popCnt);
        	 OS_eExitCriticalSection(mutexRxRb);
        	 break;

        /* API mode */
        case E_MODE_API:
        	/* read some data from ringbuffer */
        	popCnt = MIN(dataCnt, THRESHOLD_READ);

        	OS_eEnterCriticalSection(mutexRxRb);
        	ringbuffer_read(&rb_rx_spm, tmp, popCnt);
        	OS_eExitCriticalSection(mutexRxRb);
        	if (searchAtStarter(tmp, popCnt))
        	{
        		g_sDevice.eMode = E_MODE_AT;
        		PDM_vSaveRecord(&g_sDevicePDDesc);
        		uart_printf("Enter AT Mode.\r\n");
        		clear_ringbuffer(&rb_rx_spm);
        	}
        	{
        	    SPM_vProcStream(dataCnt);
        	}
        	break;
        case E_MODE_DATA:
        	/* read some data from ringbuffer */
        	popCnt = MIN(dataCnt, THRESHOLD_READ);

        	OS_eEnterCriticalSection(mutexRxRb);
        	ringbuffer_pop(&rb_rx_spm, tmp, popCnt);
        	OS_eExitCriticalSection(mutexRxRb);

        	if ((dataCnt - popCnt) >= THRESHOLD_READ)
        		OS_eActivateTask(APP_taskHandleUartRx);
        	else if ((dataCnt - popCnt) > 0)
        		vResetATimer(APP_tmrHandleUartRx, APP_TIME_MS(1));

        	/* AT filter to find AT delimiter */
        	if (searchAtStarter(tmp, popCnt))
        	{
        		g_sDevice.eMode = E_MODE_AT;
        	    PDM_vSaveRecord(&g_sDevicePDDesc);
        	    uart_printf("Enter AT Mode.\r\n");
        	    clear_ringbuffer(&rb_rx_spm);
        	}
        	/* if not containing AT, send out the data */
        	else if (g_sDevice.eState == E_NETWORK_RUN)    //Make sure network has been created.
        	{
                /* Send Data frame,call pack_lib to pack a frame */
        		memset(&apiSpec, 0, sizeof(tsApiSpec));
        		PCK_vApiSpecDataFrame(&apiSpec, 0x00, 0x00, g_sDevice.config.unicastDstAddr, tmp, popCnt);
        		memset(tmp, 0, RXFIFOLEN);
        		size = vCopyApiSpec(&apiSpec, tmp);
        		API_bSendToAirPort(g_sDevice.config.txMode, apiSpec.payload.txDataPacket.unicastAddr, tmp, size);
        	}
        	break;
        case E_MODE_MCU:
        	SPM_vProcStream(dataCnt);
        	break;
        default:break;
    }
}

/****************************************************************************
 *
 * NAME: SPM_vProcStream
 *
 * DESCRIPTION:
 * Stream Processing Machine(SPM) process stream
 * If receive a valid frame, unpack and execute callback
 * else discard it.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void SPM_vProcStream(uint32 dataCnt)
{
	uint8 tmp[RXFIFOLEN] = {0};
	/* calc the minimal */
	uint32 readCnt = MIN(dataCnt, RXFIFOLEN);

	OS_eEnterCriticalSection(mutexRxRb);
	ringbuffer_read(&rb_rx_spm, tmp, readCnt);
	OS_eExitCriticalSection(mutexRxRb);

	/* Instance an apiSpec */
	tsApiSpec apiSpec;
	bool bValid = FALSE;
	memset(&apiSpec, 0, sizeof(tsApiSpec));

	/* Deassemble apiSpec frame */
	uint16 procSize =  u16DecodeApiSpec(tmp, readCnt, &apiSpec, &bValid);
	if(!bValid)
	{
	/*
	  Invalid frame,discard from ringbuffer
	  Any data received prior to the start delimiter will be discarded.
	  If the frame is not received correctly or if the checksum fails,
	  discard too.And Re-Activate Task 1ms later.
	*/
		vResetATimer(APP_tmrHandleUartRx, APP_TIME_MS(1));
	}
	else
	{
		/* Process API frame using API support layer's api */
		API_i32UdsProcessApiCmd(&apiSpec);
	}
	/* Discard already processed part */
	OS_eEnterCriticalSection(mutexRxRb);
	ringbuffer_pop(&rb_rx_spm, tmp, procSize);
	OS_eExitCriticalSection(mutexRxRb);
}

/****************************************************************************
 *
 * NAME: SPM_vInit
 *
 * DESCRIPTION:
 * Stream Processing Machine(SPM) Init
 * Call this when you start the node,without this step,SPM will never work
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void SPM_vInit()
{
	init_ringbuffer(&rb_rx_spm, spm_rx_mempool, SPM_RX_RB_LEN);
}
