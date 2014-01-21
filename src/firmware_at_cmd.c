/*    
 * firmware_at_cmd.c
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

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "firmware_uart.h"
#include "zigbee_join.h"
#include "firmware_at_cmd.h"
#include "firmware_ota.h"
#include "zigbee_endpoint.h"


#ifndef TRACE_ATAPI
#define TRACE_ATAPI TRUE
#endif

#define PREAMBLE        0xed
#define ATHEADERLEN     4

//AT reg print functions
//int AT_printBaudRate(uint16 *regAddr); //in uart.c
int AT_printTT(uint16 *regAddr);

//AT cmd postProcess funtions:
int AT_reboot(uint16 *regAddr);
int AT_powerUpActionSet(uint16 *regAddr);
int AT_enterDataMode(uint16 *regAddr);
//int AT_reScanNetwork(uint16 *regAddr); //in zigbee_join.c
//int AT_joinNetworkWithIndex(uint16 *regAddr); //in zigbee_join.c
//int AT_setBaudRateUart1(uint16 *regAddr); //in uart.c
//int AT_listNetworkScaned(uint16 *regAddr); //in zigbee_join.c
int AT_listAllNodes(uint16 *regAddr); 
int AT_showInfo(uint16 *regAddr);
int AT_enterApiMode(uint16 *regAddr);
int AT_triggerOTAUpgrade(uint16 *regAddr);
int AT_abortOTAUpgrade(uint16 *regAddr); 
int AT_OTAStatusPoll(uint16 *regAddr); 
int AT_TestTest(uint16 *regAddr); 

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
#if 0//defined(TARGET_END)
    //for end: whether enter sleep mode
    { "SL", &g_sDevice.config.sleepMode, 1, 1, FALSE, 0, FALSE },

    //for end: wake up duration
    { "WD", &g_sDevice.config.wakeupDuration, 3, 999, FALSE, 0, FALSE }, 
#endif
    //show the infomation of node                              
    { "IF", NULL, FALSE, 0, 0, NULL, AT_showInfo },  

    //enter api mode immediatelly                              
    { "AP", NULL, FALSE, 0, 0, NULL, AT_enterApiMode }, 

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
int processSerialCmd(uint8 *buf, int len)
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
                if (atCommands[i].paramDigits == 0)
                {
                    if (atCommands[i].function != NULL)
                    {
                        result = atCommands[i].function(atCommands[i].configAddr); 
                    }
                    return result;
                }
                
                if (atCommands[i].isHex) 
                {
                    result = getHexParamData(buf, len, &paraValue, atCommands[i].paramDigits); 
                }else
                {
                    result = getDecParamData(buf, len, &paraValue, atCommands[i].paramDigits);
                }
                
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
 * NAME: AT_enterApiMode
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
int AT_enterApiMode(uint16 *regAddr)
{
    uart_printf("API mode not supported yet.\r\n");
    return ERR;
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
    
    uart_printf("FW Version       : 0x%04x \r\n", SW_VER); 

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
    uint32 br[6] = { 4800, 9600, 19200, 38400, 57600, 115200, 0 }; 
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
