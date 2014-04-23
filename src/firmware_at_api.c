/*
 * firmware_at_api.c
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

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "firmware_uart.h"
#include "zigbee_join.h"
#include "firmware_at_api.h"
#include "firmware_ota.h"
#include "zigbee_endpoint.h"
#include "firmware_hal.h"
#include "firmware_api_pack.h"
#include "firmware_cmi.h"
#include "suli.h"

#ifndef TRACE_ATAPI
#define TRACE_ATAPI TRUE
#endif

#define PREAMBLE        0xed
/* API frame delimiter */
#define API_DELIMITER 			0x7e


#define ATHEADERLEN     4

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
int API_Reboot(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);    //Reboot Callback
int API_QueryOnChipTemper(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_TestPrint(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_i32SetGpio(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr);


//AT reg print functions
int AT_printTT(uint16 *regAddr);

//AT cmd postProcess funtions:
int AT_reboot(uint16 *regAddr);
int AT_powerUpActionSet(uint16 *regAddr);
int AT_enterDataMode(uint16 *regAddr);
int AT_listAllNodes(uint16 *regAddr);
int AT_showInfo(uint16 *regAddr);
int AT_triggerOTAUpgrade(uint16 *regAddr);
int AT_abortOTAUpgrade(uint16 *regAddr);
int AT_OTAStatusPoll(uint16 *regAddr);
int AT_TestTest(uint16 *regAddr);
int AT_vQueryOnChipTemper(uint16 *regAddr);



static uint16 attt_dummy_reg = 0;

//cmd_name, reg_addr, isHex, digits, max, printFunc, func
static AT_Command_t atCommands[] =
{
    //reboot
    { "RB", NULL, FALSE, 0, 0, NULL, AT_reboot },

    //power up action, for coo:powerup re-form the network; for rou:powerup re-scan networks
    { "PA", &g_sDevice.config.powerUpAction, FALSE, 1, 1, NULL, AT_powerUpActionSet },

#ifndef TARGET_COO
    //for rou&end, auto join the first network in scan result list
    { "AJ", &g_sDevice.config.autoJoinFirst, FALSE, 1, 1, NULL, NULL },

    // re-scan radio channels to find networks
    { "RS", NULL, FALSE, 0, 0, NULL, AT_reScanNetwork },

    // list all network scaned
    { "LN", NULL, FALSE, 0, 0, NULL, AT_listNetworkScaned },

    //network index which is selected to join,MAX_SINGLE_CHANNEL_NETWORKS = 8
    { "JN", &g_sDevice.config.networkToJoin, FALSE, 3, MAX_SINGLE_CHANNEL_NETWORKS, NULL, AT_joinNetworkWithIndex },
#endif
    // list all nodes of the whole network, this will take a little more time
    { "LA", NULL, FALSE, 0, 0, NULL, AT_listAllNodes },

    //tx mode, 0: broadcast; 1:unicast
    { "TM", &g_sDevice.config.txMode, FALSE, 1, 1, NULL, NULL },

    //unicast dst addr
    { "DA", &g_sDevice.config.unicastDstAddr, TRUE, 4, 65535, NULL, NULL },

    //baud rate for uart1
    { "BR", &g_sDevice.config.baudRateUart1, FALSE, 1, 10, AT_printBaudRate, AT_setBaudRateUart1 },

    //Query On-Chip temperature
    { "QT", NULL, FALSE, 0, 0, NULL, AT_vQueryOnChipTemper},

#if 0//defined(TARGET_END)
    //for end device: whether enter sleep mode
    { "SL", &g_sDevice.config.sleepMode, 1, 1, FALSE, 0, FALSE },

    //for end: wake up duration
    { "WD", &g_sDevice.config.wakeupDuration, 3, 999, FALSE, 0, FALSE },
#endif
    //show the information of node
    { "IF", NULL, FALSE, 0, 0, NULL, AT_showInfo },

    //exit at mode into data mode
    { "EX", NULL, FALSE, 0, 0, NULL, AT_enterDataMode },
#ifdef OTA_SERVER
    //ota trigger, trigger upgrade for unicastDstAddr
    { "OT", NULL, FALSE, 0, 0, NULL, AT_triggerOTAUpgrade },

    //ota rate, client req period
    { "OR", &g_sDevice.config.reqPeriodMs, FALSE, 5, 60000, NULL, NULL },

    //ota abort
    { "OA", NULL, FALSE, 0, 0, NULL, AT_abortOTAUpgrade },

    //ota status poll
    { "OS", NULL, FALSE, 0, 0, NULL, AT_OTAStatusPoll },
#endif
    { "TT", &attt_dummy_reg, FALSE, 1, 5, AT_printTT, AT_TestTest },

};

/*
   AT command in API mode
   Notes:
         1.AT_Command_ApiMode_t returns a tsApiSpec,then response to require device;
		 2.Two branch(ApiMode/none-ApiMode) can gather together in the future;
*/
static AT_Command_ApiMode_t atCommandsApiMode[] =
{
    /* Reboot */
	{"ATRB", ATRB, NULL, FALSE, 0, 0, API_Reboot},

	/* Query local on-chip temperature */
	{ "ATQT", ATQT ,NULL, FALSE, 0, 0, API_QueryOnChipTemper},

	/* Test */
	{ "ATTP", ATTP, NULL, FALSE, 0 ,0, API_TestPrint},

	/* Set digital output */
	{"ATIO", ATIO, NULL, FALSE, 0, 0, API_i32SetGpio}
};




/****************************************************************************
 *
 * NAME: calCheckSum
 *
 * DESCRIPTION:
 * calculate a sum value of a buffer
 *
 * PARAMETERS: Name         RW  Usage
 *             in           R   pointer to buffer
 *             len          R   buffer length
 *
 * RETURNS:
 * uint8: sum value
 *
 ****************************************************************************/
uint8 calCheckSum(uint8 *in, int len)
{
    uint8 sum = 0;
    while (len-- > 0)
    {
        sum += (uint8)(*in);
        in++;
    }
    return sum;
}


/****************************************************************************
 *
 * NAME: assembleApiFrame
 *
 * DESCRIPTION:
 * assemble a tsApiFrame structure
 *
 * PARAMETERS: Name         RW  Usage
 *             frm          W   pointer to a already existing tsApiFrame sturct
 *             type         R   ENUM: teFrameType
 *             payload      R   pointer to payload
 *             payloadLen   R   payload length
 *
 * RETURNS:
 * uint16: frame length
 *
 ****************************************************************************/
uint16 assembleApiFrame(tsApiFrame *frm, teFrameType type, uint8 *payload, uint16 payloadLen)
{
    frm->preamble   = PREAMBLE;
    frm->frameType  = type;
    frm->payloadLen = payloadLen;
    memcpy(frm->payload.data, payload, payloadLen);

    frm->checksum   = calCheckSum((uint8 * )frm, 4 + payloadLen);

    return 4 + payloadLen + 1;
}

/****************************************************************************
 *
 * NAME: deassembleApiFrame
 *
 * DESCRIPTION:
 * de-assemble a tsApiFrame from a buffer
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * uint16: data length that consumed from the buffer stream
 *
 ****************************************************************************/
uint16 deassembleApiFrame(uint8 *buffer, int len, tsApiFrame *frm, bool *valid)
{
    uint8 *ptr = buffer;

    while (*ptr != PREAMBLE && len-- > 0) ptr++;
    if (len < 4)
    {
        *valid = FALSE;
        return ptr - buffer;
    }
    memcpy((uint8 * )frm, ptr, 4);
    ptr += 4;
    len -= 4;
    if (len < (frm->payloadLen+1))
    {
        *valid = FALSE;
        return ptr - buffer;
    }
    memcpy(frm->payload.data, ptr, frm->payloadLen);
    ptr += frm->payloadLen;
    frm->checksum = *ptr;
    ptr += 1;
    if (calCheckSum((uint8*)frm,4+frm->payloadLen) == frm->checksum)
    {
        *valid = TRUE;
    } else
    {
        *valid = FALSE;
    }
    return ptr-buffer;
}

/****************************************************************************
 *
 * NAME: copyApiFrame
 *
 * DESCRIPTION:
 * copy the payload part of a tsApiFrame into a buffer.
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void copyApiFrame(tsApiFrame *frm, uint8 *dst)
{
    memcpy(dst, (uint8 * )frm, 4);
    dst += 4;
    memcpy(dst, frm->payload.data, frm->payloadLen);
    dst += frm->payloadLen;
    memcpy(dst, &(frm->checksum), 1);
}

/****************************************************************************
 *
 * NAME: searchAtStarter
 *
 * DESCRIPTION:
 * search a AT command starter from a stream
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * bool: find or not
 *
 ****************************************************************************/
bool searchAtStarter(uint8 *buffer, int len)
{
    static uint8 plusCnt = 0;
    while (len--)
    {
        if (*buffer == '+')
        {
            plusCnt++;
            if (plusCnt == 3)
                return TRUE;
        }else
        {
            plusCnt = 0;
        }
        buffer++;
    }
    return FALSE;
}

/****************************************************************************
 *
 * NAME: adjustLen
 *
 * DESCRIPTION:
 * tool function for cutting a new-line ended string from a stream
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * int: position of first '\r' or '\r\n' character
 *
 ****************************************************************************/
int adjustLen(uint8 *buf, int len)
{
    int alen = 0;
    while (len-- > 0)
    {
        if (*buf == '\r' || *buf == '\r\n') break;
        alen++;
        buf++;
    }
    return alen;
}

/****************************************************************************
 *
 * NAME: getDecParamData
 *
 * DESCRIPTION:
 * tool function for internal use
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int getDecParamData(uint8 *buf, int len, uint16 *result, int size)
{
    uint8 c;
    uint16 value = 0;

    // we start to read at pos 5 as 0-1 = AT and 2-3 = CMD
    if (len == ATHEADERLEN) return NOTHING;
    if (len < ATHEADERLEN) return ERR;
    int pos = ATHEADERLEN;

    while (size-- > 0 && pos < len)
    {
        c = *(buf + (pos++));

        if ((c < '0') || (c > '9'))     // illegal char
            return ERR;
        // got a digit
        value = (value * 10) + (c - '0');
    }
    *result = value;
    return OK;

}

/****************************************************************************
 *
 * NAME: getHexParamData
 *
 * DESCRIPTION:
 * tool function for internal use
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int getHexParamData(uint8 *buf, int len, uint16 *result, int size)
{
    uint16 value = 0;
    char ar[17] = "0123456789abcdef";

    // we start to read at pos 5 as 0-1 = AT and 2-3 = CMD
    if (len == ATHEADERLEN) return NOTHING;
    if (len < ATHEADERLEN || size > 4) return ERR;


    char tmp[5];
    memcpy(tmp, buf + ATHEADERLEN, size);
    tmp[size] = '\0';

    char *low = strlwr(tmp);

    int len2 = MIN(size, (len - ATHEADERLEN));
    int i = 0;
    for ( i = 0; i < len2 ; i++)
    {
        char *pos = strchr(ar, *(low + i));
        if (pos == NULL) return ERR;
        value = value*16 + (pos-ar);
    }

    *result = value;
    return OK;
}




/****************************************************************************
 *
 * NAME: AT_reboot
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_reboot(uint16 *regAddr)
{
    vAHI_SwReset();
    return OK;
}

/****************************************************************************
 *
 * NAME: AT_powerUpActionSet
 *
 * DESCRIPTION:
 * function be executed after ATPA cmd is processed
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_powerUpActionSet(uint16 *regAddr)
{
    uart_printf("Power-up action register has been set.\r\n");
    uart_printf("You may reboot the device by reset button or ATRB cmd.\r\n");
    return OK;
}

/****************************************************************************
 *
 * NAME: AT_enterDataMode
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_enterDataMode(uint16 *regAddr)
{
    g_sDevice.eMode = E_MODE_DATA;
    PDM_vSaveRecord(&g_sDevicePDDesc);
    return OK;
}



/****************************************************************************
 *
 * NAME: AT_showInfo
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_showInfo(uint16 *regAddr)
{
    uart_printf("1.AT commands supported:\r\n");

    int cnt = sizeof(atCommands) / sizeof(AT_Command_t);
    int i = 0;
    for (i = 0; i < cnt; i++)
    {
        uart_printf("AT%s ", atCommands[i].name);
        if ((i+1) % 8 == 0)
        {
            uart_printf("\r\n");
        }
    }

    uart_printf("\r\n\r\n2.Node information:\r\n");

    uart_printf("FW Version       : 0x%04x \r\n", FW_VERSION);

    uart_printf("Short Addr       : 0x%04x \r\n", ZPS_u16AplZdoGetNwkAddr());

    uart_printf("Mac Addr         : 0x%08x%08x \r\n",
                  (uint32)(ZPS_u64AplZdoGetIeeeAddr() >> 32),
                  (uint32)(ZPS_u64AplZdoGetIeeeAddr()));

    uart_printf("RadioChnl        : %d \r\n", ZPS_u8AplZdoGetRadioChannel());

    char *txt;
    switch (ZPS_eAplZdoGetDeviceType())
    {
    case ZPS_ZDO_DEVICE_COORD:
        txt = "Co-ordinator \r\n";
        break;
    case ZPS_ZDO_DEVICE_ROUTER:
        txt = "Router \r\n";
        break;
    case ZPS_ZDO_DEVICE_ENDDEVICE:
        txt = "EndDevice \r\n";
        break;
    default:
        break;
    }
    uart_printf("Device Type      : %s",txt);

    //#define E_AHI_UART_RATE_4800       0
    //#define E_AHI_UART_RATE_9600       1
    //#define E_AHI_UART_RATE_19200      2
    //#define E_AHI_UART_RATE_38400      3
    //#define E_AHI_UART_RATE_76800      //76800's not a well-used baudrate, we take 57600 instead.
    //#define E_AHI_UART_RATE_115200     5
    uint32 br[7] = { 4800, 9600, 19200, 38400, 57600, 115200, 0};
    uint8 brIdx = g_sDevice.config.baudRateUart1;
    if (brIdx > 5)  brIdx = 6;
    uart_printf("UART1's BaudRate : %d \r\n", br[brIdx]);

    uart_printf("Unicast Dest Addr: 0x%04x \r\n", g_sDevice.config.unicastDstAddr);

    //
    txt = "\r\n\r\n3.Belonging to:\r\n";
    uart_printf(txt);

    uart_printf("PANID: 0x%04x \tEXPANID: 0x%08x%08x\r\n",
                  ZPS_u16AplZdoGetNetworkPanId(),
                  (uint32)(ZPS_u64AplZdoGetNetworkExtendedPanId() >> 32),
                  (uint32)(ZPS_u64AplZdoGetNetworkExtendedPanId()));

    return OK;
}

/****************************************************************************
 *
 * NAME: AT_triggerOTAUpgrade
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_triggerOTAUpgrade(uint16 *regAddr)
{
    if (!g_sDevice.supportOTA)
    {
        uart_printf("Node does not support OTA.\r\n");
        return ERR;
    }

    uint8 au8Values[OTA_MAGIC_NUM_LEN];
    uint32 u32TotalImage = 0;


    //first, check external flash to detect image header
    APP_vOtaFlashLockRead(OTA_MAGIC_OFFSET, OTA_MAGIC_NUM_LEN, au8Values);

    if (memcmp(magicNum, au8Values, OTA_MAGIC_NUM_LEN) == 0)
    {
        uart_printf("Found valid image at external flash.\r\n");

        //read the image length out
        APP_vOtaFlashLockRead(OTA_IMAGE_LEN_OFFSET, 4, (uint8 * )(&u32TotalImage));

        if (u32TotalImage > 256 * 1024)
        {
            uart_printf("invalid image length.\r\n");
            return ERR;
        }
    } else
    {
        uart_printf("invalid image file.\r\n");
        return ERR;
    }

    //calculate crc
    g_sDevice.otaCrc = imageCrc(u32TotalImage);
    uart_printf("Image CRC: 0x%08x.\r\n", g_sDevice.otaCrc);

    //second, notify client node
    tsFrmOtaNtf ntf;
    tsApiFrame frm;
    ntf.reqPeriodMs = g_sDevice.config.reqPeriodMs;
    ntf.totalBytes = u32TotalImage;
    g_sDevice.otaTotalBytes = ntf.totalBytes;
    g_sDevice.otaTotalBlocks = (g_sDevice.otaTotalBytes % OTA_BLOCK_SIZE == 0)?
    (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE):
    (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE + 1);

    uart_printf("Total bytes: %d, client req period: %dms \r\n", ntf.totalBytes, ntf.reqPeriodMs);

    if (sendToAir(UNICAST, g_sDevice.config.unicastDstAddr,
                  &frm, FRM_OTA_NTF, (uint8 * )(&ntf), sizeof(ntf)))
    {
        PDM_vSaveRecord(&g_sDevicePDDesc);
        return OK;
    }


    return ERR;
}

/****************************************************************************
 *
 * NAME: AT_abortOTAUpgrade
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_abortOTAUpgrade(uint16 *regAddr)
{
    uint8 dummy = 0;
    tsApiFrame frm;

    if (sendToAir(UNICAST, g_sDevice.config.unicastDstAddr,
                  &frm, FRM_OTA_ABT_REQ, (uint8 * )(&dummy), 1))
    {
        return OK;
    } else
    {
        return ERR;
    }

}

/****************************************************************************
 *
 * NAME: AT_OTAStatusPoll
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_OTAStatusPoll(uint16 *regAddr)
{
    uint8 dummy = 0;
    tsApiFrame frm;

    if (sendToAir(UNICAST, g_sDevice.config.unicastDstAddr,
                  &frm, FRM_OTA_ST_REQ, (uint8 *)(&dummy), 1))
    {
        return OK;
    }
    else
    {
        return ERR;
    }

}


/****************************************************************************
 *
 * NAME: AT_listAllNodes
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_listAllNodes(uint16 *regAddr)
{
    tsApiFrame frm;
    uint8 dummy = 0;

    if (sendToAir(BROADCAST, 0, &frm, FRM_TOPO_REQ, (&dummy), 1))
    {
        uart_printf("The request has been sent.\r\n");
        uart_printf("Wait for response...\r\n");
        return OK;
    }
    return ERR;
}

/****************************************************************************
 *
 * NAME: AT_printTT
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_printTT(uint16 *regAddr)
{
    uart_printf("ATTT cmd is for internal testing.\r\n");
    return OK;
}

/****************************************************************************
 *
 * NAME: AT_TestTest
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_TestTest(uint16 *regAddr)
{
    //disable pwm for rssi
    vAHI_TimerDisable(E_AHI_TIMER_1);
    //disable spi for external flash
    vAHI_SpiDisable();

    //now test the IOs
    bAHI_DoEnableOutputs(TRUE);  //DO0 & DO1
    vAHI_DioSetDirection((1 << 16) | (1 << 1) | (1 << 13) | (1 << DIO_ASSOC) | (1 << DIO_ON_SLEEP) | (1 << 12),
                         (1 << DIO_RSSI) | (1 << 0) | (1 << 17) | (1 << 18));
    vAHI_DoSetDataOut(0, 0x3);  //do0, do1 -> low
    vAHI_DioSetOutput((1 << 0) | (1 << 18) | (1 << DIO_RSSI), (1 << 17)); //D18 ->HIGH
    uint32 val = u32AHI_DioReadInput();
    bool ok = TRUE;
    uint16 detail = *regAddr;

    if (detail)
        uart_printf("\r\n\r\n--------------- DIO TEST ----------------\r\n");
    if (val & (1 << 13))
    {
        if (detail)
            uart_printf("Do1 = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("Do1 = 0, read 0, PASS\r\n");
    }

    if (val & (1 << DIO_ASSOC))
    {
        if (detail)
            uart_printf("RSSI = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("RSSI = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }

    if (val & (1 << DIO_ON_SLEEP))
    {
        if (detail)
            uart_printf("Do0 = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("Do0 = 0, read 0, PASS\r\n");
    }

    if (val & (1 << 12))
    {
        if (detail)
            uart_printf("D18 = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("D18 = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }

    if (val & (1 << 16))
    {
        if (detail)
            uart_printf("D17 = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("D17 = 0, read 0, PASS\r\n");
    }


    if (val & (1 << 1))
    {
        if (detail)
            uart_printf("D0 = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("D0 = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }


    vAHI_DoSetDataOut(0x3, 0);  //do0, do1 -> high
    vAHI_DioSetOutput((1 << 17), (1 << 0) | (1 << 18) | (1 << DIO_RSSI));
    val = u32AHI_DioReadInput();

    if (detail)
        uart_printf("\r\n\r\n--------------- DIO TEST 2----------------\r\n");
    if (val & (1 << 13))
    {
        if (detail)
            uart_printf("Do1 = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("Do1 = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }

    if (val & (1 << DIO_ASSOC))
    {
        if (detail)
            uart_printf("RSSI = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("RSSI = 0, read 0, PASS\r\n");
    }

    if (val & (1 << DIO_ON_SLEEP))
    {
        if (detail)
            uart_printf("Do0 = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("Do0 = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }

    if (val & (1 << 12))
    {
        if (detail)
            uart_printf("D18 = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("D18 = 0, read 0, PASS\r\n");
    }

    if (val & (1 << 16))
    {
        if (detail)
            uart_printf("D17 = 1, read 1, PASS\r\n");
    }
    else
    {
        if (detail)
            uart_printf("D17 = 1, read 0, FAIL\r\n");
        ok = FALSE;
    }

    if (val & (1 << 1))
    {
        if (detail)
            uart_printf("D0 = 0, read 1, FAIL\r\n");
        ok = FALSE;
    }
    else
    {
        if (detail)
            uart_printf("D0 = 0, read 0, PASS\r\n ");
    }

    if (!detail)
    {
        uart_printf("%s\r\n ", ok? "PASS":"FAIL");
    }

    //soft reset to restore io functions
    while (uart_get_tx_status_busy());

    vAHI_SwReset();

    return OK;
}

/****************************************************************************
 *
 * NAME: AT_queryRemoteData
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_vQueryOnChipTemper(uint16 *regAddr)
{
  /* Sample Chip Temperature */
  uint16 adSampleVal = vHAL_AdcSampleRead(E_AHI_ADC_SRC_TEMP);
  int16 i16ChipTemperature = i16HAL_GetChipTemp(adSampleVal);

  /*
	If the JN516x device operates at temperatures in excess of 90¡ãC, it may be necessary
	to call this function to maintain the frequ ency tolerance of the clock to within the
	40ppm limit specified by the IEEE 802.15. 4 standard.
  */
  vHAL_PullXtal((int32)i16ChipTemperature);

  uart_printf("On-Chip Temperature:%d C.\r\n",i16ChipTemperature);
  return OK;
}



/*------------------------------------------------------API Mode------------------------------------------------------------*/
/****************************************************************************
*
* NAME: API_Reboot
*
* DESCRIPTION:
* Warm Reboot
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
*
****************************************************************************/
int API_Reboot(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
	vAHI_SwReset();

	respApiSpec->startDelimiter = API_START_DELIMITER;
    if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
    {
    	/* Response */
    	respApiSpec->length = sizeof(tsLocalAtResp);           //Note: union length != tsLocalAtReq length
    	respApiSpec->teApiIdentifier = API_LOCAL_AT_RESP;
    	/* tsLocalAtResp package */
    	tsLocalAtResp localAtResp;
    	memset(&localAtResp, 0, sizeof(tsLocalAtResp));
    	localAtResp.atCmdId = ATRB;
    	localAtResp.eStatus = AT_OK;
    	localAtResp.frameId = reqApiSpec->payload.localAtReq.frameId;

    	respApiSpec->payload.localAtResp = localAtResp;
    	/* Calculate checkSum */
    	respApiSpec->checkSum = calCheckSum((uint8*)&localAtResp, respApiSpec->length);
    }
    else if(API_REMOTE_AT_REQ == reqApiSpec->teApiIdentifier)
    {

    }
    return OK;
}

/****************************************************************************
*
* NAME: API_QueryOnChipTemper
*
* DESCRIPTION:
* Query On-Chip temperature
* API support layer,support both local and remote mode
* Always return OK, Node never handle error, telling user to do;
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
* int
* apiSpec, returned tsApiSpec Frame
*
****************************************************************************/
int API_QueryOnChipTemper(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
  /* Sample Chip Temperature */
  uint16 adSampleVal = vHAL_AdcSampleRead(E_AHI_ADC_SRC_TEMP);
  int16 i16ChipTemperature = i16HAL_GetChipTemp(adSampleVal);

  /*
  	If the JN516x device operates at temperatures in excess of 90¡ãC, it may be necessary
  	to call this function to maintain the frequ ency tolerance of the clock to within the
  	40ppm limit specified by the IEEE 802.15. 4 standard.
  */
  vHAL_PullXtal((int32)i16ChipTemperature);

  /* Response after execute AT Command */
  retApiSpec->startDelimiter = API_START_DELIMITER;

  /* If here comes the local req */
  if(API_LOCAL_AT_REQ == inputApiSpec->teApiIdentifier)
  {
    retApiSpec->length = sizeof(tsLocalAtResp);
	retApiSpec->teApiIdentifier = API_LOCAL_AT_RESP;
    /* tsLocalAtResp package */
	tsLocalAtResp localAtResp;
	memset(&localAtResp, 0, sizeof(tsLocalAtResp));
	localAtResp.atCmdId = ATQT;
    localAtResp.eStatus = AT_OK;
    localAtResp.frameId = inputApiSpec->payload.localAtReq.frameId;
    /* This rules were specified by */
    localAtResp.value[0] = (uint8)i16ChipTemperature;    //test 4-16-1

    /* Test */
    retApiSpec->payload.localAtResp = localAtResp;
    /* Calculate checkSum */
    retApiSpec->checkSum = calCheckSum((uint8*)&localAtResp, retApiSpec->length);
  }
  else if(API_REMOTE_AT_REQ == inputApiSpec->teApiIdentifier)
  {
	retApiSpec->length = sizeof(tsRemoteAtResp);
    retApiSpec->teApiIdentifier = API_REMOTE_AT_RESP;
    tsRemoteAtResp remoteAtResp;
    memset(&remoteAtResp, 0, sizeof(tsRemoteAtResp));
    remoteAtResp.atCmdId = ATQT;
    remoteAtResp.eStatus = AT_OK;
    remoteAtResp.frameId = inputApiSpec->payload.remoteAtReq.frameId;
    remoteAtResp.value[0] = (uint8)i16ChipTemperature;
    remoteAtResp.unicastAddr = inputApiSpec->payload.remoteAtReq.unicastAddr;    //unicastAddr

    retApiSpec->payload.remoteAtResp = remoteAtResp;
    retApiSpec->checkSum = calCheckSum((uint8*)&remoteAtResp, retApiSpec->length);
  }

  return OK;
}

/****************************************************************************
*
* NAME: API_i32SetGpio
*
* DESCRIPTION:
* Set GPIO
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
* int
* apiSpec, returned tsApiSpec Frame
*
****************************************************************************/
int API_i32SetGpio(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
	 uint8 pio;
	 uint8 state;
	 IO_T  io;

	respApiSpec->startDelimiter = API_START_DELIMITER;

	/* If here comes the local req */
	if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
	{
        tsLocalAtReq localAtReq;
        memset(&localAtReq, 0, sizeof(tsLocalAtReq));
        localAtReq = reqApiSpec->payload.localAtReq;
        pio = localAtReq.value[0];
        state = localAtReq.value[1];

        /* Set GPIO through suli */
        suli_pin_init(&io, pio);
        suli_pin_dir(&io, HAL_PIN_OUTPUT);
        suli_pin_write(&io, state);

        /* Response */
        respApiSpec->length = sizeof(tsLocalAtResp);           //Note: union length != tsLocalAtReq length
        respApiSpec->teApiIdentifier = API_LOCAL_AT_RESP;
        /* tsLocalAtResp package */
        tsLocalAtResp localAtResp;
        memset(&localAtResp, 0, sizeof(tsLocalAtResp));
        localAtResp.atCmdId = ATIO;
        localAtResp.eStatus = AT_OK;
        localAtResp.frameId = reqApiSpec->payload.localAtReq.frameId;

        /* Test */
        respApiSpec->payload.localAtResp = localAtResp;
        /* Calculate checkSum */
        respApiSpec->checkSum = calCheckSum((uint8*)&localAtResp, respApiSpec->length);
	}
	else if(API_REMOTE_AT_REQ == reqApiSpec->teApiIdentifier)
	{
		respApiSpec->length = sizeof(sizeof(tsRemoteAtResp));
		tsRemoteAtReq remoteAtReq;
		memset(&remoteAtReq, 0, sizeof(tsRemoteAtReq));
		remoteAtReq = reqApiSpec->payload.remoteAtReq;
		pio = remoteAtReq.value[0];
		state = remoteAtReq.value[1];

		/* Set GPIO through suli */
		suli_pin_init(&io, pio);
		suli_pin_dir(&io, HAL_PIN_OUTPUT);
		suli_pin_write(&io, state);

		respApiSpec->teApiIdentifier = API_REMOTE_AT_RESP;
		tsRemoteAtResp remoteAtResp;
		memset(&remoteAtResp, 0, sizeof(tsRemoteAtResp));
		remoteAtResp.atCmdId = ATIO;
		remoteAtResp.eStatus = AT_OK;
		remoteAtResp.frameId = reqApiSpec->payload.remoteAtReq.frameId;

		remoteAtResp.unicastAddr = reqApiSpec->payload.remoteAtReq.unicastAddr;    //unicastAddr

		respApiSpec->payload.remoteAtResp = remoteAtResp;
		respApiSpec->checkSum = calCheckSum((uint8*)&remoteAtResp, respApiSpec->length);
	}
	return OK;
}


/* For test */
int API_TestPrint(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
	uart_printf("rev one.\r\n");
	uart_printf("S:0x%0x\n",inputApiSpec->startDelimiter);
	uart_printf("A:0x%0x\n",inputApiSpec->teApiIdentifier);
	uart_printf("len:%d\n",inputApiSpec->length);
	uart_printf("sum:%d\n",inputApiSpec->checkSum);
	return OK;
}


/****************************************************************************
*
* NAME: processSerialCmd
*
* DESCRIPTION:
*
*
* PARAMETERS: Name         RW  Usage
*
*
* RETURNS:
* void
*
****************************************************************************/
int API_i32AtProcessSerialCmd(uint8 *buf, int len)
{
    int result = ERR;

    uint16 paraValue;   // the ID used in the EEPROM

    len = adjustLen(buf, len);
    if (len < ATHEADERLEN)
        return ERRNCMD;

    // read the AT
    if (strncasecmp("AT", buf, 2) == 0)
     {
        // read the command
        int cnt = sizeof(atCommands) / sizeof(AT_Command_t);

        int i = 0;
        for (i = 0; i < cnt; i++)
        {
            // do we have a known command
            if (strncasecmp(buf + 2, atCommands[i].name, 2) == 0)
            {
            	/* There is no parameter */
                if (atCommands[i].paramDigits == 0)
                {
                    if (atCommands[i].function != NULL)
                    {
                        result = atCommands[i].function(atCommands[i].configAddr);		//like ATLA
                    }
                    return result;
                }

                if (atCommands[i].isHex)
                {
                	/* convert hex to int atoi*/
                    result = getHexParamData(buf, len, &paraValue, atCommands[i].paramDigits);
                }else
                {
                    result = getDecParamData(buf, len, &paraValue, atCommands[i].paramDigits);
                }
                /* apply value */
                if (result == NOTHING)
                {
                    if (atCommands[i].printFunc != NULL)
                        atCommands[i].printFunc(atCommands[i].configAddr);
                    else if (atCommands[i].isHex)
                        uart_printf("%04x\r\n", *(atCommands[i].configAddr));
                    else
                        uart_printf("%d\r\n", *(atCommands[i].configAddr));
                    return OK;
                }
                else if (result == OK)
                {
                    if (paraValue <= atCommands[i].maxValue)
                    {
                    	/* set value */
                        *(atCommands[i].configAddr) = paraValue;
                        PDM_vSaveRecord(&g_sDevicePDDesc);
                        if (atCommands[i].function != NULL)
                        {
                            result = atCommands[i].function(atCommands[i].configAddr);
                        }
                        return result;
                    } else
                    {
                        return OUTRNG;
                    }
                }
            }
        }
    }
    return ERRNCMD;
}

/****************************************************************************
*
* NAME: API_u8ProcessApiCmd
*
* DESCRIPTION:
* API support layer entry,Processing Api Spec Frame
*
* PARAMETERS: Name         RW  Usage
*             apiSpec      R   Api Spec Frame
* RETURNS:
* uint8 ErrorCode
*
*
****************************************************************************/
int API_i32UdsProcessApiCmd(tsApiSpec* apiSpec)
{
	int cnt = 0, i = 0;
	int result = ERR;
	int size = 0;
	uint8 tmp[sizeof(tsApiSpec)] = {0};
	uint16 txMode;
	tsLocalAtReq *localAtReq;
    tsApiSpec retApiSpec;
    memset(&retApiSpec, 0, sizeof(tsApiSpec));

	/* Process according to ApiIdentifier */
	switch(apiSpec->teApiIdentifier)
	{
	    /*
	      Local AT Require:
          1.Execute Cmd;
          2.UART DataPort ACK[tsLocalAtResp]
	    */
	    case API_LOCAL_AT_REQ:
	    	localAtReq = &(apiSpec->payload.localAtReq);

            cnt = sizeof(atCommandsApiMode)/sizeof(AT_Command_ApiMode_t);
            for(i = 0; i < cnt; i++)
            {
                if(atCommandsApiMode[i].atCmdIndex == localAtReq->atCmdId)
                {
                    if(0 == atCommandsApiMode[i].paramDigits)
                    {
                        if(NULL != atCommandsApiMode[i].function)
                        {
                            result = atCommandsApiMode[i].function(apiSpec, &retApiSpec, atCommandsApiMode[i].configAddr);
                            break;
                        }
                    }
                }
            }
        	/* UART ACK,if frameId ==0,No ACK */
            size = vCopyApiSpec(&retApiSpec, tmp);
            CMI_vTxData(tmp, size);       //pay attention to this
	    	break;
	    /*
	      remote AT Require:
	      1.Directly send to AirPort.
	    */
	    case API_REMOTE_AT_REQ:
	    	/* Option CastBit[8:2] */
            if(0 == ((apiSpec->payload.remoteAtReq.option) & OPTION_CAST_MASK))
            	txMode = UNICAST;
            else
            	txMode = BROADCAST;

            /* Send to AirPort */
            size = vCopyApiSpec(apiSpec, tmp);
	    	bool ret = API_bSendToAirPort(txMode, apiSpec->payload.remoteAtReq.unicastAddr, tmp, size);
	    	if(!ret)
	    		result = ERR;
	    	else
	    		result = OK;

	    	/* Now, don't reply here */
	        break;
	    /* TX Require */
	    case API_TX_REQ:
            /* Modify ApiIdentifier to API_RX_PACKET,let APTS process */
	    	apiSpec->teApiIdentifier = API_RX_PACKET;

	        break;
	    /* default:do nothing */
	    default:
	        break;
	}
	return result;
}

/****************************************************************************
*
* NAME: API_u8UdsProcessStackEvent
*
* DESCRIPTION:
* API support layer,Processing frame from AirPort
*
* PARAMETERS: Name          RW   Usage
*             ZPS_tsAfEvent R    StackEvent
* RETURNS:
* uint8 ErrorCode
*
*
****************************************************************************/
int API_i32AdsProcessStackEvent(ZPS_tsAfEvent sStackEvent)
{
    int cnt =0, i =0;
    int size = 0;
    int result = ERR;
    uint8 tmp[sizeof(tsApiSpec)] = {0};
    PDUM_thAPduInstance hapdu_ins;
    uint16 u16PayloadSize;
    uint8 *payload_addr;

    /* Get information from Stack Event */
    hapdu_ins = sStackEvent.uEvent.sApsDataIndEvent.hAPduInst;         //APDU
    u16PayloadSize = PDUM_u16APduInstanceGetPayloadSize(hapdu_ins);    //Payload size
    payload_addr = PDUM_pvAPduInstanceGetPayload(hapdu_ins);           //Get Payload's address

    /* Decode apiSpec frame,Now,UART and AirPort have the same structure */
    bool bValid = FALSE;
    tsApiSpec apiSpec;
    memset(&apiSpec, 0, sizeof(tsApiSpec));

    /* Decode frame from AirPort */
    u16DecodeApiSpec(payload_addr, u16PayloadSize, &apiSpec, &bValid);
    if (!bValid)
    {
        DBG_vPrintf(TRACE_NODE, "Not a valid frame, discard it.\r\n");
        PDUM_eAPduFreeAPduInstance(hapdu_ins);
        return ERR;
    }

    /* Get frame source address */
    uint16 u16SrcAddr = sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;

    tsRemoteAtReq *remoteAtReq;
    tsApiSpec respApiSpec;
    memset(&respApiSpec, 0, sizeof(tsApiSpec));

    /* Handle Tree,Call API support layer */
    switch(apiSpec.teApiIdentifier)
    {
      /*
        Remote AT require:
        1.Execute cmd
        2.AirPort ACK[tsRemoteAtResp]
      */
      case API_REMOTE_AT_REQ:
    	  /* Free APDU at first */
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);

          remoteAtReq = &(apiSpec.payload.remoteAtReq);
          cnt = sizeof(atCommandsApiMode)/sizeof(AT_Command_ApiMode_t);
          for(i = 0; i < cnt; i++)
          {
              if(atCommandsApiMode[i].atCmdIndex == remoteAtReq->atCmdId)
              {
            	  if(0 == atCommandsApiMode[i].paramDigits)
            	  {
            		  if(NULL != atCommandsApiMode[i].function)
            		  {
            			  result = atCommandsApiMode[i].function(&apiSpec, &respApiSpec, atCommandsApiMode[i].configAddr);
            		      break;
            		  }
            	  }
              }
          }
          /* ACK unicast to u16SrcAddr */
          size = vCopyApiSpec(&respApiSpec, tmp);
          bool ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
          if(!ret)
          {
        	  result = ERR;
          }
          else
          {
        	  result = OK;
          }
          break;

      /*
        Remote AT response:
        1.directly send to UART DataPort,user can handle this response frame
      */
      case API_REMOTE_AT_RESP:
    	  size = vCopyApiSpec(&apiSpec, tmp);
    	  CMI_vTxData(tmp, size);
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
          break;

      /* Data */
      case API_RX_PACKET:

    	  break;
      /* default:do nothing */
      default:
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  break;
      }
    return OK;
}

/****************************************************************************
*
* NAME: API_bSendToAirPort
*
* DESCRIPTION:
* API support layer,Call APS(application support sub-layer) to send data
*
* PARAMETERS: Name          RW   Usage
*
* RETURNS:
*
*
*
****************************************************************************/
bool API_bSendToAirPort(uint16 txMode, uint16 unicastDest, uint8 *buf, int len)
{
  PDUM_thAPduInstance hapdu_ins = PDUM_hAPduAllocateAPduInstance(apduZCL);
  /* Invalid instance */
  if(PDUM_INVALID_HANDLE == hapdu_ins)
	  return FALSE;

  uint8 *payload_addr = PDUM_pvAPduInstanceGetPayload(hapdu_ins);

  /* Copy buffer into AirPort's APDU */
  memcpy(payload_addr, buf, len);

  /* Set payload size */
  PDUM_eAPduInstanceSetPayloadSize(hapdu_ins, len);

  ZPS_teStatus st;
  if(BROADCAST == txMode)
  {
    DBG_vPrintf(TRACE_EP, "Broadcast %d ...\r\n", len);

    /* APDU will be released by the stack automatically after the APDU is send */
    st = ZPS_eAplAfBroadcastDataReq(hapdu_ins,
    		                        TRANS_CLUSTER_ID,
                                    TRANS_ENDPOINT_ID,
                                    TRANS_ENDPOINT_ID,
                                    ZPS_E_BROADCAST_ALL,
                                    SEC_MODE_FOR_DATA_ON_AIR,
                                    0,
                                    NULL);
  }
  else if(UNICAST == txMode)
  {
    DBG_vPrintf(TRACE_EP, "Unicast %d ...\r\n", len);

    st = ZPS_eAplAfUnicastDataReq(hapdu_ins,
    		                      TRANS_CLUSTER_ID,
	                              TRANS_ENDPOINT_ID,
	                              TRANS_ENDPOINT_ID,
	                              unicastDest,
	                              SEC_MODE_FOR_DATA_ON_AIR,
	                              0,
	                              NULL);
  }

  if(ZPS_E_SUCCESS != st)
  {
    /*
      In API support layer,we don't care about the failure, because handling this failure will delay or
      even block the following waiting data. So just let it go and focus on the next data.You can implement this
      mechanism in application layer.
    */
    DBG_vPrintf(TRACE_EP, "Fail to send: 0x%x, discard it... \r\n", st);
    PDUM_eAPduFreeAPduInstance(hapdu_ins);
    return FALSE;
  }
  return TRUE;
}
