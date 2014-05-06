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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
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

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_ATAPI
#define TRACE_ATAPI TRUE
#endif

#define PREAMBLE                0xed
/* API frame delimiter */
#define API_DELIMITER 			0x7e
#define ATHEADERLEN             4

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
int API_Reboot_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_RegisterSetResp_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_QueryOnChipTemper_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_i32SetGpio_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr);
int API_listAllNodes_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr);
int API_showInfo_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr);


int AT_printTT(uint16 *regAddr);
int AT_reboot(uint16 *regAddr);
int AT_powerUpActionSet(uint16 *regAddr);
int AT_enterDataMode(uint16 *regAddr);
int AT_enterApiMode(uint16 *regAddr);
int AT_enterMcuMode(uint16 *regAddr);
int AT_listAllNodes(uint16 *regAddr);
int AT_showInfo(uint16 *regAddr);
int AT_triggerOTAUpgrade(uint16 *regAddr);
int AT_abortOTAUpgrade(uint16 *regAddr);
int AT_OTAStatusPoll(uint16 *regAddr);
int AT_TestTest(uint16 *regAddr);
int AT_i32QueryOnChipTemper(uint16 *regAddr);


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static uint16 attt_dummy_reg = 0;
static uint8  dummy_value = 0;
/*
  Instruction set of AT mode
  [cmd_name, reg_addr, isHex, digits, max, printFunc, callback_func]
*/
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
    { "QT", NULL, FALSE, 0, 0, NULL, AT_i32QueryOnChipTemper},

#if 0//defined(TARGET_END)
    //for end device: whether enter sleep mode
    { "SL", &g_sDevice.config.sleepMode, 1, 1, FALSE, 0, FALSE },

    //for end: wake up duration
    { "WD", &g_sDevice.config.wakeupDuration, 3, 999, FALSE, 0, FALSE },
#endif
    //show the information of node
    { "IF", NULL, FALSE, 0, 0, NULL, AT_showInfo },

    //exit at mode into data mode
    { "DT", NULL, FALSE, 0, 0, NULL, AT_enterDataMode },

    //exit at mode into data mode
    { "AP", NULL, FALSE, 0, 0, NULL, AT_enterApiMode },

    //exit at mode into data mode
    { "MC", NULL, FALSE, 0, 0, NULL, AT_enterMcuMode },

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
   Instruction set of API mode
   Notes:
         1.CallBack AT_Command_ApiMode_t returns a tsApiSpec,then response to require device;
		 2.[Cmd_name, Cmd_index, Register, CallBack_func]
*/
static AT_Command_ApiMode_t atCommandsApiMode[] =
{
    /* Reboot */
	{ "ATRB", ATRB, NULL, API_Reboot_CallBack},

    /* Power-up action */
    { "ATPA", ATPA, &g_sDevice.config.powerUpAction, API_RegisterSetResp_CallBack },

    /* auto join */
    { "ATAJ", ATAJ, &g_sDevice.config.autoJoinFirst, API_RegisterSetResp_CallBack },

    /* Re-Scan network */
    { "ATRS", ATRS, NULL, API_RescanNetwork_CallBack },

    /* tx mode */
    { "ATTM", ATTM, &g_sDevice.config.txMode, API_RegisterSetResp_CallBack },

    /* Join network with its index */
    { "ATJN", ATJN, &g_sDevice.config.networkToJoin, API_JoinNetworkWithIndex_CallBack },

    /* Unicast dest address*/
    { "ATDA", ATDA, &g_sDevice.config.unicastDstAddr, API_RegisterSetResp_CallBack },

    /* Baud Rate of UART1 */
    { "ATBR", ATBR, &g_sDevice.config.baudRateUart1, API_RegisterSetResp_CallBack },

	/* Query local on-chip temperature */
	{ "ATQT", ATQT ,NULL, API_QueryOnChipTemper_CallBack},

	/* Set digital output */
	{"ATIO", ATIO, NULL, API_i32SetGpio_CallBack},

	//{ "LA", ATLA, NULL, API_listAllNodes },    //special ApiIdentifier

#ifndef TARGET_COO
	{ "LN", ATLN, NULL, API_listNetworkScaned_CallBack },
#endif

	{ "IF", ATIF, NULL, API_showInfo_CallBack },

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
        if (*buf == '\r' || *buf == '\n') break;
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
    for (i = 0; i < len2; i++)
    {
        char *pos = strchr(ar, *(low + i));
        if (pos == NULL) return ERR;
        value = value * 16 + (pos - ar);
    }

    *result = value;
    return OK;
}


/****************************************************************************
 *
 * NAME: assembleLocalAtResp
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
int assembleLocalAtResp(tsLocalAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len)
{
    resp->frameId = frm_id;
    resp->atCmdId = cmd_id;
    resp->eStatus = status;
    memcpy(resp->value, value, len);
    return sizeof(tsLocalAtResp);
}

/****************************************************************************
 *
 * NAME: assembleRemoteAtResp
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
int assembleRemoteAtResp(tsRemoteAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len, uint16 addr)
{
    resp->frameId = frm_id;
    resp->atCmdId = cmd_id;
    resp->eStatus = status;
    memcpy(resp->value, value, len);
    resp->unicastAddr = addr;
    return sizeof(tsRemoteAtResp);
}

/****************************************************************************
 *
 * NAME: assembleApiSpec
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
void assembleApiSpec(tsApiSpec *api, uint8 idtf, uint8 *payload, int payload_len)
{
    api->startDelimiter = API_START_DELIMITER;
    api->length = payload_len;
    api->teApiIdentifier = idtf;
    memcpy((uint8 *)&api->payload.localAtReq, payload, payload_len);
    api->checkSum = calCheckSum(payload, payload_len);
}


/****************************************************************************
 *
 * NAME: postReboot
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
void postReboot()
{
    if (g_sDevice.rebootByRemote && g_sDevice.eState > E_NETWORK_STARTUP)
    {
        tsApiSpec apiSpec;
        uint8 tmp[sizeof(tsApiSpec)] = {0};
        int len = assembleRemoteAtResp(&apiSpec.payload.remoteAtResp, 0, ATRB, AT_OK, tmp, 1, g_sDevice.rebootByAddr);
        apiSpec.startDelimiter = API_START_DELIMITER;
        apiSpec.length = len;
        apiSpec.teApiIdentifier = API_REMOTE_AT_RESP;
        apiSpec.checkSum = calCheckSum((uint8 *)&apiSpec.payload.remoteAtResp, len);
        int size = i32CopyApiSpec(&apiSpec, tmp);
        API_bSendToAirPort(UNICAST, apiSpec.payload.remoteAtResp.unicastAddr, tmp, size);

    } else if (!g_sDevice.rebootByRemote)
    {
        if (g_sDevice.eMode == E_MODE_AT)
        {
            uart_printf("OK");
        } else if (g_sDevice.eMode == E_MODE_API)
        {
            tsApiSpec apiSpec;
            uint8 tmp[sizeof(tsApiSpec)] = { 0 };
            int len = assembleLocalAtResp(&apiSpec.payload.localAtResp, 0, ATRB, AT_OK, tmp, 1);
            apiSpec.startDelimiter = API_START_DELIMITER;
            apiSpec.length = len;
            apiSpec.teApiIdentifier = API_LOCAL_AT_RESP;
            apiSpec.checkSum = calCheckSum((uint8 *)&apiSpec.payload.localAtResp, len);
            int size = i32CopyApiSpec(&apiSpec, tmp);
            CMI_vTxData(tmp, size);
        }
    }

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
    g_sDevice.rebootByCmd = true;
    g_sDevice.rebootByRemote = false;
    PDM_vSaveRecord(&g_sDevicePDDesc);

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
    uart_printf("Enter Data Mode.\r\n");
    return OK;
}

/* Enter API mode */
int AT_enterApiMode(uint16 *regAddr)
{
    g_sDevice.eMode = E_MODE_API;
    PDM_vSaveRecord(&g_sDevicePDDesc);
    uart_printf("Enter API Mode.\r\n");
    return OK;
}

/* Enter MCU mode */
int AT_enterMcuMode(uint16 *regAddr)
{
    g_sDevice.eMode = E_MODE_MCU;
    PDM_vSaveRecord(&g_sDevicePDDesc);
    uart_printf("Enter MCU Mode.\r\n");
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

    char *txt = "";
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

    uint32 br[7] = { 4800, 9600, 19200, 38400, 57600, 115200, 0};
    uint8 brIdx = g_sDevice.config.baudRateUart1;
    if (brIdx > 5)  brIdx = 6;
    uart_printf("UART1's BaudRate : %d \r\n", br[brIdx]);

    uart_printf("Unicast Dest Addr: 0x%04x \r\n", g_sDevice.config.unicastDstAddr);

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

    /* check external flash to detect image header at first */
    APP_vOtaFlashLockRead(OTA_MAGIC_OFFSET, OTA_MAGIC_NUM_LEN, au8Values);

    if (memcmp(magicNum, au8Values, OTA_MAGIC_NUM_LEN) == 0)
    {
        uart_printf("Found valid image at external flash.\r\n");

        /* read the image length */
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

    /* calculate crc */
    g_sDevice.otaCrc = imageCrc(u32TotalImage);
    uart_printf("Image CRC: 0x%08x.\r\n", g_sDevice.otaCrc);

    /* server notify client,here comes an OTA upgrade event */
    tsOtaNotice otaNotice;
    memset(&otaNotice, 0, sizeof(tsOtaNotice));

    uint8 tmp[sizeof(tsApiSpec)] = {0};
    tsApiSpec apiSpec;
    memset(&apiSpec, 0, sizeof(tsApiSpec));

    /* package OtaNotice */
    otaNotice.reqPeriodMs = g_sDevice.config.reqPeriodMs;
    otaNotice.totalBytes = u32TotalImage;

    /* package ApiSpec */
    apiSpec.startDelimiter = API_START_DELIMITER;
    apiSpec.length = sizeof(tsOtaNotice);
    apiSpec.teApiIdentifier = API_OTA_NTC;
    apiSpec.payload.otaNotice = otaNotice;
    apiSpec.checkSum = calCheckSum((uint8*)&otaNotice, apiSpec.length);

    /* calculate how many blocks of this OTA image */
    g_sDevice.otaTotalBytes = otaNotice.totalBytes;
    g_sDevice.otaTotalBlocks = (g_sDevice.otaTotalBytes % OTA_BLOCK_SIZE == 0)?
    (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE):
    (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE + 1);

    uart_printf("Total bytes: %d, client req period: %dms \r\n", otaNotice.totalBytes, otaNotice.reqPeriodMs);

    /* send through AirPort */
    int size = i32CopyApiSpec(&apiSpec, tmp);
    if(API_bSendToAirPort(UNICAST, g_sDevice.config.unicastDstAddr, tmp, size))
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
	uint8 tmp[sizeof(tsApiSpec)]={0};
    tsApiSpec apiSpec;
    memset(&apiSpec, 0, sizeof(tsApiSpec));

	apiSpec.startDelimiter = API_START_DELIMITER;
	apiSpec.length = 1;
	apiSpec.teApiIdentifier = API_OTA_ABT_REQ;
	apiSpec.payload.dummyByte = 0;
	apiSpec.checkSum = 0;

	 /* send through AirPort */
	int size = i32CopyApiSpec(&apiSpec, tmp);
	if(API_bSendToAirPort(UNICAST, g_sDevice.config.unicastDstAddr, tmp, size))
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
int AT_OTAStatusPollbak(uint16 *regAddr)
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

int AT_OTAStatusPoll(uint16 *regAddr)
{
	uint8 tmp[sizeof(tsApiSpec)]={0};
    tsApiSpec apiSpec;
    memset(&apiSpec, 0, sizeof(tsApiSpec));

	apiSpec.startDelimiter = API_START_DELIMITER;
	apiSpec.length = 1;
	apiSpec.teApiIdentifier = API_OTA_ST_REQ;
	apiSpec.payload.dummyByte = 0;
	apiSpec.checkSum = 0;

	 /* send through AirPort */
	int size = i32CopyApiSpec(&apiSpec, tmp);
	if(API_bSendToAirPort(UNICAST, g_sDevice.config.unicastDstAddr, tmp, size))
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
	uint8 tmp[sizeof(tsApiSpec)] = {0};
    tsApiSpec apiSpec;
    memset(&apiSpec, 0, sizeof(tsApiSpec));

    tsNwkTopoReq nwkTopoReq;
    memset(&nwkTopoReq, 0, sizeof(tsNwkTopoReq));

    nwkTopoReq.reqCmd = dummy_value;

    /* Package Nwk Topo require apiSpec */
    apiSpec.startDelimiter = API_START_DELIMITER;
    apiSpec.length = sizeof(tsNwkTopoReq);
    apiSpec.teApiIdentifier = API_TOPO_REQ;
    apiSpec.payload.nwkTopoReq = nwkTopoReq;
    apiSpec.checkSum = calCheckSum((uint8*)&nwkTopoReq, apiSpec.length);

    int size = i32CopyApiSpec(&apiSpec, tmp);
  	if(API_bSendToAirPort(BROADCAST, 0, tmp, size))
  	{
  		uart_printf("The request has been sent.\r\n");
		uart_printf("Waiting for response...\r\n");
		return OK;
  	}
  	else
  	{
  		return ERR;
  	}

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
 * NAME: AT_i32QueryOnChipTemper
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
int AT_i32QueryOnChipTemper(uint16 *regAddr)
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




/****************************************************************************
*
* NAME: API_Reboot_CallBack
*
* DESCRIPTION:
* Warm Reboot
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
*
****************************************************************************/
int API_Reboot_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
    g_sDevice.rebootByCmd = true;
    if (API_REMOTE_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        g_sDevice.rebootByRemote = true;
        g_sDevice.rebootByAddr = inputApiSpec->payload.remoteAtReq.unicastAddr;
    }
    else if (API_LOCAL_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        g_sDevice.rebootByRemote = false;
    }
    PDM_vSaveRecord(&g_sDevicePDDesc);
	vAHI_SwReset();
    return OK;
}

/****************************************************************************
*
* NAME: API_RegisterSetResp_CallBack
*
* DESCRIPTION:
*
*
* PARAMETERS: Name         RW  Usage
*
* RETURNS:
*
****************************************************************************/
int API_RegisterSetResp_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr)
{
    //save the register
    if (inputApiSpec->payload.localAtReq.value[0] != 0)
    {
        memcpy((uint8 *)regAddr, inputApiSpec->payload.localAtReq.value + 1, 2);
        PDM_vSaveRecord(&g_sDevicePDDesc);
    }

    if (API_LOCAL_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsLocalAtResp localAtResp;
        int len = assembleLocalAtResp(&localAtResp,
                                      inputApiSpec->payload.localAtReq.frameId,
                                      inputApiSpec->payload.localAtReq.atCmdId,
                                      AT_OK, (uint8 *)regAddr, 2);
        assembleApiSpec(retApiSpec, API_LOCAL_AT_RESP, (uint8 *)&localAtResp, len);
    }
    else if (API_REMOTE_AT_REQ == inputApiSpec->teApiIdentifier)
    {
        tsRemoteAtResp remoteAtResp;
        int len = assembleRemoteAtResp(&remoteAtResp,
                                       inputApiSpec->payload.remoteAtReq.frameId,
                                       inputApiSpec->payload.remoteAtReq.atCmdId,
                                       AT_OK, (uint8 *)regAddr, 2, inputApiSpec->payload.remoteAtReq.unicastAddr);
        assembleApiSpec(retApiSpec, API_REMOTE_AT_RESP, (uint8 *)&remoteAtResp, len);
    }
    return OK;
}


/****************************************************************************
*
* NAME: API_QueryOnChipTemper_CallBack
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
int API_QueryOnChipTemper_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
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
    uint8 val = (uint8)i16ChipTemperature;

	if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
	{
		tsLocalAtResp localAtResp;
		memset(&localAtResp, 0, sizeof(tsLocalAtResp));

		/* Assemble LocalAtResp */
		assembleLocalAtResp(&localAtResp,
							reqApiSpec->payload.localAtReq.frameId,
							ATQT,
							AT_OK,
							&val,
							sizeof(uint8));

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
							ATQT,
							AT_OK,
							&val,
							sizeof(uint8),
							reqApiSpec->payload.remoteAtReq.unicastAddr);

	   /* Assemble apiSpec */
	   assembleApiSpec(respApiSpec,
					  API_REMOTE_AT_RESP,
					  (uint8*)&remoteAtResp,
					  sizeof(tsRemoteAtResp));
	}
	return OK;
}

/****************************************************************************
*
* NAME: API_i32SetGpio_CallBack
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
int API_i32SetGpio_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
	 uint8 pio;
	 uint8 state;
	 IO_T  io;
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
	     tsLocalAtResp localAtResp;
	     memset(&localAtResp, 0, sizeof(tsLocalAtResp));

		 /* Assemble LocalAtResp */
		 assembleLocalAtResp(&localAtResp,
							 reqApiSpec->payload.localAtReq.frameId,
							 ATIO,
							 AT_OK,
							 &dummy_value,
							 1);

		 /* Assemble apiSpec */
		 assembleApiSpec(respApiSpec,
						API_LOCAL_AT_RESP,
						(uint8*)&localAtResp,
						sizeof(tsLocalAtResp));

	 }
	 else if(API_REMOTE_AT_REQ == reqApiSpec->teApiIdentifier)
	 {
		 tsRemoteAtReq remoteAtReq;
		 memset(&remoteAtReq, 0, sizeof(tsRemoteAtReq));
	 	 remoteAtReq = reqApiSpec->payload.remoteAtReq;
	 	 pio = remoteAtReq.value[0];
	 	 state = remoteAtReq.value[1];

		 /* Set GPIO through suli */
		 suli_pin_init(&io, pio);
		 suli_pin_dir(&io, HAL_PIN_OUTPUT);
		 suli_pin_write(&io, state);

		 /* Response */
		 tsRemoteAtResp remoteAtResp;
		 memset(&remoteAtResp, 0, sizeof(tsRemoteAtResp));

	 	 /* Assemble RemoteAtResp */
		 assembleRemoteAtResp(&remoteAtResp,
							 reqApiSpec->payload.remoteAtReq.frameId,
							 ATIO,
							 AT_OK,
							 &dummy_value,
							 1,
							 reqApiSpec->payload.remoteAtReq.unicastAddr);

	    /* Assemble apiSpec */
	    assembleApiSpec(respApiSpec,
					   API_REMOTE_AT_RESP,
					   (uint8*)&remoteAtResp,
					   sizeof(tsRemoteAtResp));
	 	}
	 return OK;
}


/****************************************************************************
*
* NAME: API_listAllNodes_CallBack
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
int API_listAllNodes_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
	tsTopoInfo topoInfo;
	memset(&topoInfo, 0, sizeof(topoInfo));

	topoInfo.nodeMacAddr0 = (uint32)ZPS_u64AplZdoGetIeeeAddr();
	topoInfo.nodeMacAddr1 = (uint32)(ZPS_u64AplZdoGetIeeeAddr() >> 32);
	topoInfo.nodeFWVer = (uint16)(FW_VERSION);

	if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
	{
		tsLocalAtResp localAtResp;
		memset(&localAtResp, 0, sizeof(tsLocalAtResp));

		/* Assemble LocalAtResp */
		assembleLocalAtResp(&localAtResp,
				            reqApiSpec->payload.localAtReq.frameId,
				            ATLA,
				            AT_OK,
				            (uint8*)&topoInfo,
				            sizeof(tsTopoInfo));

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
        		             (uint8*)&topoInfo,
        		             sizeof(tsTopoInfo),
        		             reqApiSpec->payload.remoteAtReq.unicastAddr);

        /* Assemble apiSpec */
        assembleApiSpec(respApiSpec,
						API_REMOTE_AT_RESP,
						(uint8*)&remoteAtResp,
						sizeof(tsRemoteAtResp));
	}
	return OK;
}

/****************************************************************************
*
* NAME: API_showInfo_CallBack
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
int API_showInfo_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr)
{
	tsNodeInfo nodeInfo;
	memset(&nodeInfo, 0, sizeof(tsNodeInfo));

	/* Get information */
	nodeInfo.nodeFWVer = (uint16)(FW_VERSION);
	nodeInfo.shortAddr = ZPS_u16AplZdoGetNwkAddr();
 	nodeInfo.nodeMacAddr0 = (uint32)ZPS_u64AplZdoGetIeeeAddr();
    nodeInfo.nodeMacAddr1 = (uint32)(ZPS_u64AplZdoGetIeeeAddr() >> 32);
    nodeInfo.radioChannel = ZPS_u8AplZdoGetRadioChannel();
    nodeInfo.role = (uint8)ZPS_eAplZdoGetDeviceType();
    nodeInfo.panId = (uint16)ZPS_u16AplZdoGetNetworkPanId();

    /* Response */
	if(API_LOCAL_AT_REQ == reqApiSpec->teApiIdentifier)
	{
		tsLocalAtResp localAtResp;
		memset(&localAtResp, 0, sizeof(tsLocalAtResp));

		/* Assemble LocalAtResp */
		assembleLocalAtResp(&localAtResp,
							reqApiSpec->payload.localAtReq.frameId,
							ATIF,
							AT_OK,
							(uint8*)&nodeInfo,
							sizeof(tsNodeInfo));

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
							 ATIF,
							 AT_OK,
							 (uint8*)&nodeInfo,
							 sizeof(tsNodeInfo),
							 reqApiSpec->payload.remoteAtReq.unicastAddr);

		/* Assemble apiSpec */
		assembleApiSpec(respApiSpec,
						API_REMOTE_AT_RESP,
						(uint8*)&remoteAtResp,
						sizeof(tsRemoteAtResp));
	}
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
int API_i32AtCmdProc(uint8 *buf, int len)
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
* NAME: API_i32ApiFrmCmdProc
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
int API_i32ApiFrmCmdProc(tsApiSpec* apiSpec)
{
	int i = 0;
	int cnt = 0;
	int size = 0;
	int result = ERR;
	uint8 tmp[sizeof(tsApiSpec)] = {0};
	uint16 txMode;
	bool ret;

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
	    {
	    	tsLocalAtReq *localAtReq = &(apiSpec->payload.localAtReq);

            cnt = sizeof(atCommandsApiMode)/sizeof(AT_Command_ApiMode_t);
            for(i = 0; i < cnt; i++)
            {
                if(atCommandsApiMode[i].atCmdIndex == localAtReq->atCmdId)
                {
                    if(NULL != atCommandsApiMode[i].function)
                    {
                        result = atCommandsApiMode[i].function(apiSpec, &retApiSpec, atCommandsApiMode[i].configAddr);
                        break;
                    }
                }
            }
        	/* UART ACK,if frameId ==0,No ACK(not implement in v1003) */
            size = i32CopyApiSpec(&retApiSpec, tmp);
            CMI_vTxData(tmp, size);       //pay attention to this
            break;
	    }

	    /*
	      remote AT Require:
	      1.Directly send to AirPort.
	    */
	    case API_REMOTE_AT_REQ:
	    {
	    	/* Option CastBit[8:2] */
            if(0 == ((apiSpec->payload.remoteAtReq.option) & OPTION_CAST_MASK))
            	txMode = UNICAST;
            else
            	txMode = BROADCAST;

            /* Send to AirPort */
            size = i32CopyApiSpec(apiSpec, tmp);
	    	ret = API_bSendToAirPort(txMode, apiSpec->payload.remoteAtReq.unicastAddr, tmp, size);
	    	if(!ret)
	    		result = ERR;
	    	else
	    		result = OK;
	    	/* Now, don't reply here in local device */
	    	break;
	    }

	    /*
	      TX Data packet require(not in transparent mode but MCU or API mode)
	      1.Send to unicast address directly.
	    */
	    case API_DATA_PACKET:
	    {
	    	if(0 == ((apiSpec->payload.txDataPacket.option) & OPTION_CAST_MASK))
				txMode = UNICAST;
			else
				txMode = BROADCAST;
            /* Send to AirPort */
	    	size = i32CopyApiSpec(apiSpec, tmp);
	    	ret = API_bSendToAirPort(txMode, apiSpec->payload.txDataPacket.unicastAddr, tmp, size);
			if(!ret)
				result = ERR;
			else
				result = OK;
			break;
	    }
	}
	return result;
}

/****************************************************************************
*
* NAME: API_i32AdsStackEventProc
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
int API_i32AdsStackEventProc(ZPS_tsAfEvent *sStackEvent)
{
    int cnt =0, i =0;
    int size = 0;
    int result = ERR;
    bool ret = ERR;
    uint8 tmp[sizeof(tsApiSpec)] = {0};
    PDUM_thAPduInstance hapdu_ins;
    uint16 u16PayloadSize;
    uint8 *payload_addr;

    uint8 lqi;
    uint16 pwmWidth;

    /* Adapt RSSI Led */
    lqi = sStackEvent->uEvent.sApsDataIndEvent.u8LinkQuality;

    pwmWidth = lqi * 500 /110;
    if(pwmWidth > 500)
    	pwmWidth = 500;
    vAHI_TimerStartRepeat(E_AHI_TIMER_1, 500 - pwmWidth, 500 + pwmWidth);

    /* Get information from Stack Event */
    hapdu_ins = sStackEvent->uEvent.sApsDataIndEvent.hAPduInst;         //APDU
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
    uint16 u16SrcAddr = sStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;

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
      {
    	  /* Free APDU at first */
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);

          cnt = sizeof(atCommandsApiMode)/sizeof(AT_Command_ApiMode_t);
          for(i = 0; i < cnt; i++)
          {
        	  if(atCommandsApiMode[i].atCmdIndex == apiSpec.payload.remoteAtReq.atCmdId)
              {
                  if(NULL != atCommandsApiMode[i].function)
                  {
                      result = atCommandsApiMode[i].function(&apiSpec, &respApiSpec, atCommandsApiMode[i].configAddr);
                      break;
                  }
              }
          }
          /* ACK unicast to u16SrcAddr */
          size = i32CopyApiSpec(&respApiSpec, tmp);
          ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
          if(!ret)
        	  result = ERR;
          else
        	  result = OK;
          break;
      }

      /*
        Remote AT response:
        1.directly send to UART DataPort,user can handle this response frame
      */
      case API_REMOTE_AT_RESP:
      {
    	  size = i32CopyApiSpec(&apiSpec, tmp);
    	  CMI_vTxData(tmp, size);
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  result = OK;
    	  break;
      }

      /* Data */
      case API_DATA_PACKET:
      {
    	  size = i32CopyApiSpec(&apiSpec, tmp);
          CMI_vTxData(tmp, size);
          PDUM_eAPduFreeAPduInstance(hapdu_ins);
          result = OK;
          break;
      }

      /*
        Nwk Topo require:
        1.Get link Quality,dbm,firmware version,mac
        2.AirPort ACK to source address
      */
      case API_TOPO_REQ:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
          DBG_vPrintf(TRACE_EP, "NWK_TOPO_REQ: from 0x%04x \r\n", u16SrcAddr);

          /* Pack a NwkTopoResp,AirPort ACK */
          tsNwkTopoResp nwkTopoResp;
          memset(&nwkTopoResp, 0, sizeof(nwkTopoResp));

          /* Fill in the parameter */
          nwkTopoResp.dbm = (lqi - 305) / 3;
          nwkTopoResp.lqi = lqi;
          nwkTopoResp.nodeFWVer = (uint16)(FW_VERSION);
          nwkTopoResp.shortAddr = (uint16)ZPS_u16AplZdoGetNwkAddr();              //Short Address
          nwkTopoResp.nodeMacAddr0 = (uint32)ZPS_u64AplZdoGetIeeeAddr();          //Low
          nwkTopoResp.nodeMacAddr1 = (uint32)(ZPS_u64AplZdoGetIeeeAddr() >> 32);  //High

          respApiSpec.startDelimiter = API_START_DELIMITER;
          respApiSpec.length = sizeof(tsNwkTopoResp);
          respApiSpec.teApiIdentifier = API_TOPO_RESP;
          respApiSpec.payload.nwkTopoResp = nwkTopoResp;
          respApiSpec.checkSum = calCheckSum((uint8*)&nwkTopoResp, respApiSpec.length);

          /* ACK unicast to u16SrcAddr */
          size = i32CopyApiSpec(&respApiSpec, tmp);
		  ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
		  if(!ret)
		    result = ERR;
		  else
		    result = OK;
		  break;
      }

      /*
        Nwk Topo Response
        1.Send to CMI directly,let CMI handle this.
      */
      case API_TOPO_RESP:
      {
    	  size = i32CopyApiSpec(&apiSpec, tmp);
		  CMI_vTxData(tmp, size);
		  PDUM_eAPduFreeAPduInstance(hapdu_ins);
		  result = OK;
		  break;
      }

#ifdef OTA_CLIENT
      /*
        OTA notice message
        1.Save parameter from notice message;
        2.Erase external flash;
        3.Activate require Task
      */
      case API_OTA_NTC:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  if (!g_sDevice.supportOTA)
    		  break;
          g_sDevice.otaReqPeriod  = apiSpec.payload.otaNotice.reqPeriodMs;
          g_sDevice.otaTotalBytes = apiSpec.payload.otaNotice.totalBytes;
          g_sDevice.otaSvrAddr16  = u16SrcAddr;
          g_sDevice.otaCurBlock   = 0;
          g_sDevice.otaTotalBlocks = (g_sDevice.otaTotalBytes % OTA_BLOCK_SIZE == 0) ?
                                     (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE) :
                                     (g_sDevice.otaTotalBytes / OTA_BLOCK_SIZE + 1);
          g_sDevice.otaDownloading = 1;
          DBG_vPrintf(TRUE, "OTA_NTC: %d blks \r\n", g_sDevice.otaTotalBlocks);
          PDM_vSaveRecord(&g_sDevicePDDesc);

          /* erase covered sectors */
          APP_vOtaFlashLockEraseAll();

          /* Activate OTA require Task */
          OS_eActivateTask(APP_taskOTAReq);
          result = OK;
          break;
      }

      /*
        OTA response hold a block
        1. Write this block into external flash.
        2. If this is the last block, activate upgrade.
      */
      case API_OTA_RESP:
      {
          PDUM_eAPduFreeAPduInstance(hapdu_ins);
          uint32 blkIdx = apiSpec.payload.otaResp.blockIdx;
          uint32 offset = blkIdx * OTA_BLOCK_SIZE;
          uint16 len    = apiSpec.payload.otaResp.len;
          uint32 crc    = apiSpec.payload.otaResp.crc;

          /* Synchronous blocks */
          if (blkIdx == g_sDevice.otaCurBlock)
          {
              DBG_vPrintf(TRUE, "OTA_RESP: Blk: %d\r\n", blkIdx);
              APP_vOtaFlashLockWrite(offset, len, apiSpec.payload.otaResp.block);
              g_sDevice.otaCurBlock += 1;
              g_sDevice.otaCrc = crc;
              if (g_sDevice.otaCurBlock % 100 == 0)
                  PDM_vSaveRecord(&g_sDevicePDDesc);
          }
          else
          {
              DBG_vPrintf(TRUE, "OTA_RESP: DesireBlk: %d, RecvBlk: %d \r\n", g_sDevice.otaCurBlock, blkIdx);
          }

          /* if this is the last block,client start to upgrade */
          if (g_sDevice.otaCurBlock >= g_sDevice.otaTotalBlocks)
          {
              clientOtaFinishing();
          }
          result = OK;
          break;
      }

      /*
        OTA upgrade response
        1.Allowed to activate the upgrade by server
      */
      case API_OTA_UPG_RESP:
      {
          PDUM_eAPduFreeAPduInstance(hapdu_ins);
          DBG_vPrintf(TRACE_EP, "FRM_OTA_UPG_RESP: from 0x%04x \r\n", u16SrcAddr);

          g_sDevice.otaDownloading = 0;
          PDM_vSaveRecord(&g_sDevicePDDesc);

          APP_vOtaKillInternalReboot();
          result = OK;
          break;
      }

      /*
        Client received OTA abort command from server
        1.Set OTA state machine to zero(IDLE).
        2.Reset parameter of OTA.
        3.Response to server.
      */
      case API_OTA_ABT_REQ:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
		  DBG_vPrintf(TRUE, "FRM_OTA_ABT_REQ: from 0x%04x \r\n", u16SrcAddr);
		  if (g_sDevice.otaDownloading > 0)
		  {
			  g_sDevice.otaDownloading = 0;
			  g_sDevice.otaCurBlock = 0;
			  g_sDevice.otaTotalBytes = 0;
			  g_sDevice.otaTotalBlocks = 0;
			  PDM_vSaveRecord(&g_sDevicePDDesc);
		  }

    	  /* package apiSpec */
    	  respApiSpec.startDelimiter = API_START_DELIMITER;
    	  respApiSpec.length = 1;
    	  respApiSpec.teApiIdentifier = API_OTA_ABT_RESP;
    	  respApiSpec.payload.dummyByte = 0;
    	  respApiSpec.checkSum = 0;

		  /* send through AirPort */
		  size = i32CopyApiSpec(&respApiSpec, tmp);
		  ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
          if(!ret)
        	  result = ERR;
          else
        	  result = OK;
          break;
      }


      /*
        OTA status require:
        1.
      */
      case API_OTA_ST_REQ:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
		  DBG_vPrintf(TRACE_EP, "FRM_OTA_ST_REQ: from 0x%04x \r\n", u16SrcAddr);

		  tsOtaStatusResp otaStatusResp;
		  otaStatusResp.inOTA = (g_sDevice.otaDownloading > 0);
		  otaStatusResp.per = 0;
		  if (otaStatusResp.inOTA && g_sDevice.otaTotalBlocks > 0)
		  {
			  otaStatusResp.per = (uint8)((g_sDevice.otaCurBlock * 100) / g_sDevice.otaTotalBlocks);
			  otaStatusResp.min = g_sDevice.config.reqPeriodMs * (g_sDevice.otaTotalBlocks - g_sDevice.otaCurBlock) / 60000;
		  }

		  /* response */
		  respApiSpec.startDelimiter = API_START_DELIMITER;
		  respApiSpec.length = sizeof(tsOtaStatusResp);
		  respApiSpec.teApiIdentifier = API_OTA_ST_RESP;
		  respApiSpec.payload.otaStatusResp = otaStatusResp;
		  respApiSpec.checkSum = calCheckSum((uint8*)&otaStatusResp, respApiSpec.length);

		  /* ACK unicast to u16SrcAddr */
		  size = i32CopyApiSpec(&respApiSpec, tmp);
		  ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
		  if(!ret)
			result = ERR;
		  else
			result = OK;
		  break;
      }

#endif

#ifdef OTA_SERVER
      /*
        OTA data block require
        1. return block data
      */
      case API_OTA_REQ:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  uint32 blkIdx  = apiSpec.payload.otaReq.blockIdx;
		  if (blkIdx >= g_sDevice.otaTotalBlocks)
		 	  break;

		  uint8 buff[OTA_BLOCK_SIZE];
		  uint16 rdLen = ((blkIdx + 1) * OTA_BLOCK_SIZE > g_sDevice.otaTotalBytes) ?
						 (g_sDevice.otaTotalBytes - blkIdx * OTA_BLOCK_SIZE) :
						 (OTA_BLOCK_SIZE);
		  if (rdLen > OTA_BLOCK_SIZE)
			  rdLen = OTA_BLOCK_SIZE;

		  /* read a block from flash */
		  APP_vOtaFlashLockRead(blkIdx * OTA_BLOCK_SIZE, rdLen, buff);

		  DBG_vPrintf(TRUE, "OTA_REQ: blkIdx: %d \r\n", blkIdx);

		  tsOtaResp resp;
		  resp.blockIdx = blkIdx;
		  memcpy(&resp.block[0], buff, rdLen);
		  resp.len = rdLen;
		  resp.crc = g_sDevice.otaCrc;

		  respApiSpec.startDelimiter = API_START_DELIMITER;
		  respApiSpec.length = sizeof(tsOtaResp);
		  respApiSpec.teApiIdentifier = API_OTA_RESP;
		  respApiSpec.payload.otaResp = resp;
		  respApiSpec.checkSum = calCheckSum((uint8*)&resp, respApiSpec.length);

		  /* ACK unicast to u16SrcAddr */
		  size = i32CopyApiSpec(&respApiSpec, tmp);
		  ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
		  if(!ret)
			result = ERR;
		  else
			result = OK;
		  break;
      }

       /*
         Upgrade require from OTA client device
         1.Permit client to activate upgrade
       */
      case API_OTA_UPG_REQ:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  DBG_vPrintf(TRUE, "FRM_OTA_UPG_REQ: from 0x%04x \r\n", u16SrcAddr);
    	  uart_printf("OTA: Node 0x%04x's OTA download done, crc check ok.\r\n", u16SrcAddr);

    	  /* package apiSpec */
    	  respApiSpec.startDelimiter = API_START_DELIMITER;
    	  respApiSpec.length = 1;
    	  respApiSpec.teApiIdentifier = API_OTA_UPG_RESP;
    	  respApiSpec.payload.dummyByte = 0;
    	  respApiSpec.checkSum = 0;

		  /* send through AirPort */
		  size = i32CopyApiSpec(&respApiSpec, tmp);
		  ret = API_bSendToAirPort(UNICAST, u16SrcAddr, tmp, size);
          if(!ret)
        	  result = ERR;
          else
        	  result = OK;
          break;
      }

      /* Telling server, abort OK */
      case API_OTA_ABT_RESP:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
		  DBG_vPrintf(TRACE_EP, "FRM_OTA_ABT_RESP: from 0x%04x \r\n", u16SrcAddr);
		  uart_printf("OTA: abort ack from 0x%04x.\r\n", u16SrcAddr);
		  result = OK;
		  break;
      }

      /* interact with user */
      case API_OTA_ST_RESP:
      {
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
		  DBG_vPrintf(TRACE_EP, "FRM_OTA_ST_RESP: from 0x%04x \r\n", u16SrcAddr);
		  if (apiSpec.payload.otaStatusResp.inOTA)
		  {
			  uart_printf(" -------------------- \r\n");
			  uart_printf("     OTA status       \r\n");
              uart_printf(" Node: 0x%04x         \r\n", u16SrcAddr);
              uart_printf(" Finished: %d%%       \r\n", apiSpec.payload.otaStatusResp.per);
              uart_printf(" Remaining: %ld min   \r\n", apiSpec.payload.otaStatusResp.min);
			  uart_printf(" -------------------- \r\n");
		  }
		  else
		  {
			  uart_printf("OTA: Node 0x%04x's is not in OTA or OTA finished.\r\n");
		  }
		  break;
      }

#endif

      /* default:free APDU only */
      default:
    	  PDUM_eAPduFreeAPduInstance(hapdu_ins);
    	  result = OK;
    	  break;
      }
    return result;
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

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
