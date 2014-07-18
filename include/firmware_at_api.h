/*
 * firmware_at_api.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log :
 * [2014/03/20 oliver]add API support layer.
 * [2014/04/10 oliver]merge apiFrame and apiSpec
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

#ifndef __AT_API_H__
#define __AT_API_H__

#include <jendefs.h>
#include "firmware_uart.h"
#include "firmware_ota.h"

/* macro define */

#define AT_REQ_PARAM_LEN      4         //maximal size of AT parameter
#define AT_RESP_PARAM_LEN     20        //maximal size of AT response hex value
#define API_DATA_LEN          32        //maximal size of each API data frame
#define AT_CMD_LEN            8         //AT command length

#define API_START_DELIMITER   0x7e   //API special frame start delimiter

#define OPTION_CAST_MASK      0x40    //option unicast or broadcast MASK
#define OPTION_ACK_MASK       0x80    //option ACK or not MASK

/*
  API mode index
  Note: Concept of AT command instruction set and apiIdentifier is different.
*/
typedef enum
{
    ATRB = 0x30,  //reboot
    ATPA = 0x32,  //power up action, for coo:powerup re-form the network; for rou:powerup re-scan networks
    ATAJ = 0x34,  //for rou&end, auto join the first network in scan result list
    ATRS = 0x36,  //re-scan radio channels to find networks
    ATLN = 0x38,  //list all network scaned
    ATJN = 0x40,  //network index which is selected to join,MAX_SINGLE_CHANNEL_NETWORKS = 8
    ATRJ = 0x41,  //rejoin the last network
    ATLA = 0x42,  //list all nodes of the whole network, this will take a little more time
    ATTM = 0x44,  //tx mode, 0: broadcast; 1:unicast
    ATDA = 0x46,  //unicast dst addr
    ATBR = 0x48,  //baud rate for uart1
    ATQT = 0x50,  //query on-chip temperature
    ATQV = 0x52,  //query on-chip voltage
    ATIF = 0x54,  //show information of the node
    ATAP = 0x56,  //enter API mode
    ATEX = 0x58,  //exit API mode,end data mode
    ATOT = 0x60,  //ota trigger, trigger upgrade for unicastDstAddr
    ATOR = 0x62,  //ota rate, client req period
    ATOA = 0x64,  //ota abort
    ATOS = 0x66,  //ota status poll
    ATTP = 0x68,  //for test
    ATIO = 0x70,  //set IOs
    ATAD = 0x72   //read ADC value from AD1 AD2 AD3 AD4
}teAtIndex;

/* API mode AT return value */
typedef enum
{
    AT_OK = 0,
    AT_ERR = 1,
    INVALID_CMD = 2,
    INVALID_PARAM = 3
}teAtRetVal;

/* GPIO R/W flag */
enum eGpioRw
{
    GPIO_RD = 1,
    GPIO_WR = 0
};
/* adapt RPC in the future */
typedef enum
{
    /* API identifier */
    API_LOCAL_AT_REQ = 0x08,      //local At require
    API_LOCAL_AT_RESP = 0x88,    //local At response
    API_REMOTE_AT_REQ = 0x17,    //remote At require
    API_REMOTE_AT_RESP = 0x97,   //remote At response
    API_DATA_PACKET = 0x02,      //indicate that's a data packet,data packet is certainly remote packet.
    API_TEST = 0x8f,             //Test
    /* recently */
    API_OTA_NTC = 0xd3,
    API_OTA_REQ = 0xb0,
    API_OTA_RESP = 0x06,
    API_OTA_ABT_REQ = 0xf7,
    API_OTA_ABT_RESP = 0xdb,
    API_OTA_UPG_REQ = 0x5a,
    API_OTA_UPG_RESP = 0xe6,
    API_OTA_ST_REQ = 0x91,
    API_OTA_ST_RESP = 0x89,
    API_TOPO_REQ = 0xfb,
    API_TOPO_RESP = 0x6b
}teApiIdentifier;


/*--------API mode structure--------*/
/* Information of topology */
typedef struct
{
    uint8               lqi;
    uint8               dbm;
    uint16              nodeFWVer;
    uint32              nodeMacAddr0;  //can not sent 64bit int due to align issue
    uint32              nodeMacAddr1;
}__attribute__ ((packed)) tsTopoInfo;


/* Information of Node */
typedef struct
{
    uint8               role;
    uint8               radioChannel;
    uint16              nodeFWVer;
    uint16              shortAddr;
    uint16              panId;
    uint32              nodeMacAddr0;  //cant sent 64bit interger due to align issue
    uint32              nodeMacAddr1;
}__attribute__ ((packed)) tsNodeInfo;

/* Information of Network */
typedef struct
{
    uint8 index;
    uint8 radioChannel;
    uint8 isPermitJoin;
    uint32 panId0;
    uint32 panId1;
}__attribute__ ((packed)) tsNwkInfo;


/* API mode local AT Command require */
typedef struct
{
    uint8 frameId;                  //identifies the UART data frame to correlate with subsequent ACK
    uint8 atCmdId;                  //AT Command index
    uint8 value[AT_REQ_PARAM_LEN];  //if present,indicates the requested parameter value to set
                                    //the given register,if no character present,register is queried
}__attribute__ ((packed)) tsLocalAtReq;


/* API mode local AT Command response */
typedef struct
{
    uint8 frameId;                      //identifies the UART data frame to correlate with subsequent ACK
    uint8 atCmdId;                      //AT Command index
    uint8 eStatus;                      //OK,ERROR,Invalid command,Invalid Parameter
    uint8 value[AT_RESP_PARAM_LEN];     //value returned in hex format
}__attribute__ ((packed)) tsLocalAtResp;


/* API mode remote AT Command require */
typedef struct
{
    uint8 frameId;
    uint8 option;                //0x01 Disable ACK,default: 0x00 Enable ACK
    uint8 atCmdId;
    uint8 value[AT_REQ_PARAM_LEN];
    uint16 unicastAddr;
    uint64 unicastAddr64;
}__attribute__ ((packed)) tsRemoteAtReq;

/* API mode remote AT command response */
typedef struct
{
    uint8 frameId;
    uint8 atCmdId;
    uint8 eStatus;
    uint16 unicastAddr;
    uint64 unicastAddr64;
    uint8 valueLen;
    uint8 value[AT_RESP_PARAM_LEN];
}__attribute__ ((packed)) tsRemoteAtResp;

/* Tx data packet 24 */
typedef struct
{
    uint8 frameId;
    uint8 option;             //Indicate broadcast or unicast,need ACK or not.
    uint16 unicastAddr;       //unicast address
    uint64 unicastAddr64;
    uint8 dataLen;            //data length in data array
    uint8 data[API_DATA_LEN]; //data array
}__attribute__ ((packed)) tsTxDataPacket;

/* ATLA,list all nodes in network */
typedef struct
{
    uint8 reqCmd;
}__attribute__ ((packed)) tsNwkTopoReq;

/* Network Topology response */
typedef struct
{
    uint8  lqi;
    int16  dbm;
    uint16 nodeFWVer;
    uint16 shortAddr;      //indicate this response comes from which node
    uint32 nodeMacAddr0;
    uint32 nodeMacAddr1;
}__attribute__ ((packed)) tsNwkTopoResp;

/* notice message of OTA server */
typedef struct
{
    uint32 totalBytes;    //total bytes of this OTA image
    uint16 reqPeriodMs;   //require period time of client
}__attribute__ ((packed)) tsOtaNotice;

/* OTA require */
typedef struct
{
    uint32 blockIdx;
}__attribute__ ((packed)) tsOtaReq;

/* OTA response */
typedef struct
{
    uint32 blockIdx;
    uint16 len;
    uint8  block[OTA_BLOCK_SIZE];
    uint32 crc;
}__attribute__ ((packed)) tsOtaResp;

/* OTA status */
typedef struct
{
    bool   inOTA;
    uint8  per;    //percent
    uint32 min;    //Remaining time
}__attribute__ ((packed)) tsOtaStatusResp;

/* GPIO parameter */
typedef struct
{
    uint8 rw;      //read/write flag
    uint8 pio;     //pin number
    uint8 state;   //state of pio
}__attribute__ ((packed)) tsGpio;

/* ADC structure */
typedef struct
{
    uint8 src;
    uint16 value;
}__attribute__ ((packed)) tsAdc;

/* API-specific structure */
typedef struct
{
    uint8 startDelimiter;   // start delimiter '0x7e'
    uint8 length;           // length = sizeof(payload)
    uint8 teApiIdentifier;  //indicate what type of packets this is
    union
    {
        /*diff app frame*/
        uint8 dummyByte;            //dummy byte for non-information frame
        tsNwkTopoReq nwkTopoReq;
        tsNwkTopoResp nwkTopoResp;
        tsLocalAtReq localAtReq;
        tsLocalAtResp localAtResp;
        tsRemoteAtReq remoteAtReq;
        tsRemoteAtResp remoteAtResp;
        tsTxDataPacket txDataPacket;
        tsOtaNotice    otaNotice;    //OTA notice message
        tsOtaReq otaReq;
        tsOtaResp otaResp;
        tsOtaStatusResp otaStatusResp;
    }__attribute__ ((packed)) payload;
    uint8 checkSum;                             //verify byte
}__attribute__ ((packed)) tsApiSpec;


//AT
typedef int (*AT_Command_Function_t)(uint16 *);
typedef int (*AT_Command_Print_t)(uint16 *);
typedef int (*AT_CommandApiMode_Func_t)(tsApiSpec*, tsApiSpec*, uint16*);
typedef int8 byte;

typedef struct
{
    const char    *name;
    uint16        *configAddr;   // the ID used in the EEPROM
    const bool    isHex;             // whether the reg number is hex
    const int     paramDigits;        // how many digits for the parameter
    const uint16  maxValue;           // maximum value of the parameter
    AT_Command_Print_t    printFunc;  //the print function of this reg
    AT_Command_Function_t function;   // the function which does the real work on change
}  AT_Command_t;


/* AT_Command in API mode */
typedef struct
{
    const char  *name;                  //AT command name
    uint8       atCmdIndex;             //AT command index
    uint16      *configAddr;            //config address
    AT_CommandApiMode_Func_t function;  //AT commands call back function
}AT_Command_ApiMode_t;

/* return code */
enum ErrorCode
{
    OK,
    ERR,
    NOTHING,
    MODIFIED,
    OUTRNG,
    OKREBOOT,
    ERRNCMD
};

/****************************************************************************/
/***        Global Function Prototypes                                    ***/
/****************************************************************************/
uint8 calCheckSum(uint8 *in, int len);
bool searchAtStarter(uint8 *buffer, int len);
int assembleLocalAtResp(tsLocalAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len);
int assembleRemoteAtResp(tsRemoteAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len);
void assembleApiSpec(tsApiSpec *api, uint8 idtf, uint8 *payload, int payload_len);

int API_i32AtCmdProc(uint8 *buf, int len);
int API_i32ApiFrmProc(tsApiSpec* apiSpec);
int API_i32AdsStackEventProc(ZPS_tsAfEvent *sStackEvent);
bool API_bSendToAirPort(uint16 txMode, uint16 unicastDest, uint8 *buf, int len);
bool API_bSendToEndPoint(uint16 txMode, uint16 unicastDest, uint8 srcEpId, uint8 dstEpId, char *buf, int len);
bool API_bSendToMacDev(uint64 unicastMacAddr, uint8 srcEpId, uint8 dstEpId, char *buf, int len);  /*[Override]*/
void postReboot();

#endif /* __AT_API_H__ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
