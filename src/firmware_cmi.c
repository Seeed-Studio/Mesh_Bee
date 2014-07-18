/*
 * firmware_cmi.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Oliver Wang
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
#include <jendefs.h>
#include "firmware_cmi.h"
#include "firmware_uart.h"
#include "firmware_ringbuffer.h"
#include "firmware_api_pack.h"
#include "common.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_CMI
#define TRACE_CMI FALSE
#endif

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        External Function Prototypes                                     ***/
/****************************************************************************/
extern uint32 SPM_u32PullData(void *data, int len);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: CMI_vUrtRevDataDistributor
 *
 * DESCRIPTION:
 * Communication interface layer
 * The most important thing to note: CMI layer is the transport center of all data
 * ADS and UDS send the data to CMI directly,then CMI choose a data route(like a router
 * map)
 *
 * PARAMETERS: Name         RW  Usage
 *             data         R   Data from UART1
 *
 * RETURNS:
 * bool: TRUE - busy
 *
 ****************************************************************************/
void CMI_vUrtRevDataDistributor(void *data, int len)
{
    uint32 avlb_cnt = 0;    //avlb count
    uint32 free_cnt = 0;
    uint32 min_cnt = 0;
    /*
     * In different mode,data will flow to different ringbuffer, switched by CMI
     * AT: HardUart->SPM data pool (Call API support layer to analyze console input character)
     * DATA: HardUart->SPM data pool(Send to air directly)
     * API: HardUart->SPM data pool(Call API support layer to unpack frame and execute)
     * MCU: HartUart->UPS ringbuffer(user will handle this)
     * In order to making data flow more clear,we choose (switch/case) rather than (if/else)
    */
    switch(g_sDevice.eMode)
    {
        /* AT mode */
        case E_MODE_AT:
            avlb_cnt = SPM_u32PullData(data, len);
            break;

        /* API mode */
        case E_MODE_API:
            avlb_cnt = SPM_u32PullData(data, len);
            break;

        /* DATA mode */
        case E_MODE_DATA:
            avlb_cnt = SPM_u32PullData(data, len);
            break;

        /* Arduino-ful MCU mode */
        case E_MODE_MCU:
            OS_eEnterCriticalSection(mutexRxRb);
            free_cnt = ringbuffer_free_space(&rb_uart_aups);
            OS_eExitCriticalSection(mutexRxRb);

            min_cnt = MIN(free_cnt, len);
            DBG_vPrintf(TRACE_CMI, "aups_rb, rev_cnt: %u, free_cnt: %u \r\n", len, free_cnt);
            /* If ringbuffer is full,don't push */
            if(min_cnt > 0)
            {
                OS_eEnterCriticalSection(mutexRxRb);
                ringbuffer_push(&rb_uart_aups, data, min_cnt);
                OS_eExitCriticalSection(mutexRxRb);
            }
            break;

        /* default:do nothing */
        default:break;
    }

    /*
    * the following mechanism is to improve the effective of every ZigBee packet frame
    * by avoiding sending packet that is too short.
    */
    if(E_MODE_MCU != g_sDevice.eMode)
    {
        if (avlb_cnt >= THRESHOLD_READ)
        {
            OS_eActivateTask(APP_taskHandleUartRx);             //Activate SPM immediately
        }
        else
        {
            vResetATimer(APP_tmrHandleUartRx, APP_TIME_MS(5));  //Activate SPM 5ms later
        }
    }
}

/****************************************************************************
 *
 * NAME: CMI_vLocalAckDistributor
 *
 * DESCRIPTION:
 * Communication interface layer
 *
 * PARAMETERS: Name         RW  Usage
 *             apiSpec      R   tsApiSpec frame
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
void CMI_vLocalAckDistributor(tsApiSpec *apiSpec)
{
    uint8 tmp[sizeof(tsApiSpec)] = {0};
    uint32 len = 0;

    len = i32CopyApiSpec(apiSpec, tmp);

    switch(g_sDevice.eMode)
    {
        /* API mode */
        case E_MODE_API:
        {
            /* Mechanism: wait until ringbuffer has enough space */
            uart_tx_data(tmp, len);
            break;
        }
        case E_MODE_MCU:
        {
            len = i32CopyApiSpec(apiSpec, tmp);
            OS_eEnterCriticalSection(mutexAirPort);
            uint32 free_cnt = ringbuffer_free_space(&rb_air_aups);
            OS_eExitCriticalSection(mutexAirPort);

            /* if free size < len, discard it */
            if(free_cnt >= len)
            {
                OS_eEnterCriticalSection(mutexAirPort);
                ringbuffer_push(&rb_air_aups, tmp, len);
                OS_eExitCriticalSection(mutexAirPort);
            }
            break;
        }
        /* Only in MCU/API mode, UART need ACK */
        default:break;
    }
}


/****************************************************************************
 *
 * NAME: CMI_vAirDataDistributor
 *
 * DESCRIPTION:
 * Communication interface layer
 * AirPort Received data, this module will choose a data flow
 * PARAMETERS: Name         RW  Usage
 *             data         R   Data from UART1
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
void CMI_vAirDataDistributor(tsApiSpec *apiSpec)
{
    uint8 tmp[sizeof(tsApiSpec)] = {0};

    uint32 len = 0;

    switch(g_sDevice.eMode)
    {
        /* AT mode */
        case E_MODE_AT:
        {
            /* distribute to console */
            if(API_TOPO_RESP == apiSpec->teApiIdentifier)
            {
                tsNwkTopoResp nwkTopoResp;
                memset(&nwkTopoResp, 0, sizeof(tsNwkTopoResp));
                nwkTopoResp = apiSpec->payload.nwkTopoResp;

                DBG_vPrintf(TRACE_EP, "NWK_TOPO_RESP: from 0x%04x \r\n", nwkTopoResp.shortAddr);

                uart_printf("+--Node resp--\r\n|--0x%04x,%08lx%08lx,LQI:%d,DBm:%d,Ver:0x%04x\r\n",
                           nwkTopoResp.shortAddr,
                           (uint32)nwkTopoResp.nodeMacAddr1,
                           (uint32)nwkTopoResp.nodeMacAddr0,
                           nwkTopoResp.lqi,
                           nwkTopoResp.dbm,
                           nwkTopoResp.nodeFWVer);
            }
            break;
        }
        /* API mode */
        case E_MODE_API:
        {
            /* Mechanism: wait until ringbuffer has enough space */
               len = i32CopyApiSpec(apiSpec, tmp);
               uart_tx_data(tmp, len);
            break;
        }
        /* DATA mode */
        case E_MODE_DATA:
        {
            /* Mechanism: wait until ringbuffer has enough space */
            uart_tx_data(apiSpec->payload.txDataPacket.data, apiSpec->payload.txDataPacket.dataLen);
            break;
        }
        /* MCU mode */
        case E_MODE_MCU:
        {
            len = i32CopyApiSpec(apiSpec, tmp);
            OS_eEnterCriticalSection(mutexAirPort);
            uint32 free_cnt = ringbuffer_free_space(&rb_air_aups);
            OS_eExitCriticalSection(mutexAirPort);

            /* if free size < len, discard it */
            if(free_cnt >= len)
            {
                OS_eEnterCriticalSection(mutexAirPort);
                ringbuffer_push(&rb_air_aups, tmp, len);
                OS_eExitCriticalSection(mutexAirPort);
            }
            break;
        }
   }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
