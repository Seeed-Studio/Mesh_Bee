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
 *
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

#define AT_PARAM_LEN      8 		//maximal size of AT parameter
#define AT_PARAM_HEX_LEN  20		//maximal size of AT response hex value
#define API_DATA_LEN      20		//maximal size of each API data frame
#define AT_CMD_LEN        8			//AT command length

#define API_START_DELIMITER  0x7e	//API special frame start delimiter

#define OPTION_CAST_MASK     0x40    //option unicast or broadcast MASK
#define OPTION_ACK_MASK      0x80    //option ACK or not MASK

/*
  API mode index
  Note: Concept of AT command instruction set and apiIdentifier
*/
typedef enum
{
	ATRB = 0x30,  //reboot
	ATPA = 0x32,  //power up action, for coo:powerup re-form the network; for rou:powerup re-scan networks
	ATAJ = 0x34,  //for rou&end, auto join the first network in scan result list
	ATRS = 0x36,  //re-scan radio channels to find networks
	ATLN = 0x38,  //list all network scaned
    ATJN = 0x40,  //network index which is selected to join,MAX_SINGLE_CHANNEL_NETWORKS = 8
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
    ATTP = 0x68,   //for test
    ATIO = 0x70
}teAtIndex;

typedef enum
{
    FRM_CTRL,
    FRM_QUERY,
    FRM_QUERY_RESP,
    FRM_DATA,
    FRM_OTA_NTF,
    FRM_OTA_REQ,
    FRM_OTA_RESP,
    FRM_OTA_ABT_REQ,
    FRM_OTA_ABT_RESP,
    FRM_OTA_UPG_REQ,
    FRM_OTA_UPG_RESP,
    FRM_OTA_ST_REQ,
    FRM_OTA_ST_RESP,
    FRM_TOPO_REQ,
    FRM_TOPO_RESP
}teFrameType;

//Query type
typedef enum
{
	QUERY_INNER_TEMP = 10,				//on-chip temperature
	QUERY_INNER_VOL = 11				//on-chip voltage
	/*your sensor data type here*/
}teQueryType;

/* API mode AT return enum */
typedef enum
{
	AT_OK = 0,
	AT_ERR = 1,
	INVALID_CMD = 2,
	INVALID_PARAM = 3
}teAtRetVal;

/* adapt RPC */
typedef enum
{
	/* API identifier */
	API_LOCAL_AT_REQ = 0x08,
	API_LOCAL_AT_RESP = 0x88,
	API_REMOTE_AT_REQ = 0x17,
	API_REMOTE_AT_RESP = 0x97,
	API_DATA_PACKET = 0x02,
    API_TX_REQ = 0x01,          //Tx a packet to special short address
    API_TX_RESP = 0x03,
	API_RX_PACKET = 0x81,        //received a packet from air,send to UART
	API_TEST = 0x8f,				//Test
	/* recently */
	API_CTRL = 0xe7,
	API_QUERY = 0xbe,
	API_QUERY_RESP = 0x43,
	API_DATA = 0xc0,
	API_OTA_NTF = 0xd3,
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

//CTRL
typedef struct
{
    uint8               reg[2];
    uint8               value[16];
}tsFrmControl;

//OTA
typedef struct
{
    uint32              totalBytes;
    uint16              reqPeriodMs;
}tsFrmOtaNtf;

typedef struct
{
    uint32              blockIdx;
}tsFrmOtaReq;

typedef struct
{
    uint32              blockIdx;
    uint16              len;
    uint8               block[OTA_BLOCK_SIZE];
    uint32              crc;
}tsFrmOtaResp;

typedef struct
{
    bool                inOTA;
    uint8               per;
}tsFrmOtaStatusResp;

//TOPO
typedef struct
{
    uint32              nodeMacAddr0;  //cant sent 64bit interger due to align issue
    uint32              nodeMacAddr1;
    uint16              nodeFWVer;
}tsFrmTOPOResp;

//API frame, external layer structure
typedef struct __apiFrame
{
    uint8               preamble;
    uint8               frameType;
    uint16              payloadLen;
    union
    {
        uint8           data[RXFIFOLEN];
        tsFrmControl    frmCtrl;
        tsFrmOtaNtf     frmOtaNtf;
        tsFrmOtaReq     frmOtaReq;
        tsFrmOtaResp    frmOtaResp;
        tsFrmOtaStatusResp    frmOtaStResp;
        tsFrmTOPOResp   frmTopoResp;
    }payload;
    uint8               checksum;
}tsApiFrame;


/*--------API mode structure--------*/

/* API mode local AT Command require 10 byte */
typedef struct
{
	uint8 frameId;              //identifies the UART data frame to correlate with subsequent ACK
	uint8 atCmdId;              //AT Command index
	uint8 value[AT_PARAM_LEN];  //if present,indicates the requested parameter value to set
							    //the given register,if no character present,register is queried
}__attribute__ ((packed)) tsLocalAtReq;


/* API mode local AT Command response 23 */
typedef struct
{
	uint8 frameId;				    //identifies the UART data frame to correlate with subsequent ACK
	uint8 atCmdId;		            //AT Command index
	uint8 eStatus;				    //OK,ERROR,Invalid command,Invalid Parameter
	uint8 value[AT_PARAM_HEX_LEN];  //value returned in hex format
}__attribute__ ((packed)) tsLocalAtResp;


/* API mode remote AT Command require 13 */
typedef struct
{
	uint8 frameId;
	uint8 option;                //0x01 Disable ACK,default: 0x00 Enable ACK
	uint8 atCmdId;
	uint8 value[AT_PARAM_LEN];
	uint16 unicastAddr;          //from small to large
}__attribute__ ((packed)) tsRemoteAtReq;


/* API mode remote AT command response 25 */
typedef struct
{
	uint8 frameId;
	uint8 atCmdId;
	uint8 eStatus;
	uint8 value[AT_PARAM_HEX_LEN];
	uint16 unicastAddr;
}__attribute__ ((packed)) tsRemoteAtResp;


/* Tx data packet 24 */
typedef struct
{
	uint8 frameId;
	uint8 option;             //indicate broadcast or unicast
	uint8 dataLen;
	uint8 data[API_DATA_LEN];
	uint16 unicastAddr;
}__attribute__ ((packed)) tsTxDataPacket;


/* API-specific structure */
typedef struct
{
	uint8 startDelimiter;
	uint8 length;           // length = sizeof(payload)
	uint8 teApiIdentifier;
	union
	{
		/*diff app frame*/
		tsLocalAtReq localAtReq;
		tsLocalAtResp localAtResp;
		tsRemoteAtReq remoteAtReq;
		tsRemoteAtResp remoteAtResp;
		tsTxDataPacket txDataPacket;            //No ACK, like UDP
	}__attribute__ ((packed)) payload;
	uint8 checkSum;
}__attribute__ ((packed)) tsApiSpec;

/*---------------End---------------*/

//AT
typedef int (*AT_Command_Function_t)(uint16 *);
typedef int (*AT_Command_Print_t)(uint16 *);
typedef int (*AT_CommandApiMode_Func_t)(tsApiSpec*, tsApiSpec*, uint16*);
typedef int8 byte;

typedef struct
{
    const char    *name;
    uint16        *configAddr;   // the ID used in the EEPROM
    const bool    isHex;         // whether the reg number is hex
    const int     paramDigits;  // how many digits for the parameter
    const uint16  maxValue;     // maximum value of the parameter
    AT_Command_Print_t    printFunc;  //the print function of this reg
    AT_Command_Function_t function; // the function which does the real work on change
}  AT_Command_t;


/* AT_Command in API mode */
typedef struct
{
	const char	*name;		            //AT command name
	uint8       atCmdIndex;             //AT command index
	uint16		*configAddr;            //config address
	const bool	isHex;
	const int 	paramDigits;
	const uint16 maxValue;
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
uint16 assembleApiFrame(tsApiFrame *frm, teFrameType type, uint8 *payload, uint16 payloadLen);
uint16 deassembleApiFrame(uint8 *buffer, int len, tsApiFrame *frm, bool *valid);
uint16 u16DecodeApiSpec(uint8 *buffer, int len, tsApiSpec *spec, bool *valid);		//oliver add for API mode
void copyApiFrame(tsApiFrame *frm, uint8 *dst);
bool searchAtStarter(uint8 *buffer, int len);

void assembleLocalAtResp(tsLocalAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len); 
void assembleRemoteAtResp(tsRemoteAtResp *resp, uint8 frm_id, uint8 cmd_id, uint8 status, uint8 *value, int len, uint16 addr);
void assembleApiSpec(tsApiSpec *api, uint8 len, uint8 idtf, uint8 *payload, int payload_len); 

int API_i32AtProcessSerialCmd(uint8 *buf, int len);
int API_i32UdsProcessApiCmd(tsApiSpec* apiSpec);
int API_i32AdsProcessStackEvent(ZPS_tsAfEvent sStackEvent);
bool API_bSendToAirPort(uint16 txMode, uint16 unicastDest, uint8 *buf, int len);
#endif /* __AT_API_H__ */
