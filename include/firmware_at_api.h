/*
 * firmware_at_api.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log :
 * [2014/03/20 oliver]add api mode.
 * [2014/04/09 oliver]take endian into consideration,divide unicastAddr into two parts.
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

typedef enum
{
    FRM_CTRL,
    FRM_QUERY,
    FRM_QUERY_RESP,		//oliver add
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

typedef enum
{
	/* API identifier */
	API_LOCAL_AT_REQ = 0x08,
	API_LOCAL_AT_RESP = 0x88,
	API_REMOTE_AT_REQ = 0x17,
	API_REMOTE_AT_RESP = 0x97,
    API_TX_REQ = 0x01,          //Tx a packet to special short address
    API_TX_RESP = 0x03,
	API_RX_PACKET = 0x81,        //received a packet from air,send to UART
	API_TEST = 0x8f				//Test
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

/* API mode local AT Command require */
typedef struct
{
	uint8 frameId;              //identifies the UART data frame to correlate with subsequent ACK
	uint8 atCmd[AT_CMD_LEN];    //AT Command name,four ASCII char
	uint8 value[AT_PARAM_LEN];  //if present,indicates the requested parameter value to set
							    //the given register,if no character present,register is queried
}__attribute__ ((packed)) tsLocalAtReq;


/* API mode local AT Command response */
typedef struct
{
	uint8 frameId;				    //identifies the UART data frame to correlate with subsequent ACK
	uint8 atCmd[AT_CMD_LEN];		//AT Command name,four ASCII char
	uint8 eStatus;				    //OK,ERROR,Invalid command,Invalid Parameter
	uint8 value[AT_PARAM_HEX_LEN];  //value returned in hex format
}__attribute__ ((packed)) tsLocalAtResp;


/* API mode remote AT Command require */
typedef struct
{
	uint8 frameId;
	uint8 unicastAddrH;
	uint8 unicastAddrL;
	uint8 atCmd[AT_CMD_LEN];
	uint8 value[AT_PARAM_LEN];
}__attribute__ ((packed)) tsRemoteAtReq;


/* API mode remote AT command response */
typedef struct
{
	uint8 frameId;
	uint8 unicastAddrH;
	uint8 unicastAddrL;
	uint8 atCmd[AT_CMD_LEN];
	uint8 eStatus;
	uint8 value[AT_PARAM_HEX_LEN];
}__attribute__ ((packed)) tsRemoteAtResp;


/* Tx data packet */
typedef struct
{
	uint8 frameId;
	uint8 unicastAddrH;
	uint8 unicastAddrL;
	uint8 option;
	uint8 data[API_DATA_LEN];
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
		tsTxDataPacket txDataPacket;
	}payload;
	uint8 checkSum;
}__attribute__ ((packed)) tsApiSpec;
/*---------------End---------------*/

//AT
typedef int (*AT_Command_Function_t)(uint16 *);
typedef int (*AT_Command_Print_t)(uint16 *);
typedef int (*AT_CommandApiMode_Func_t)(tsApiSpec*, uint16*);
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

/* AT_Command in API mode oliver */
typedef struct
{
	const char	*name;		//AT command name
	uint16		*configAddr;	//config address
	const bool	isHex;
	const int 	paramDigits;
	const uint16 maxValue;
	AT_CommandApiMode_Func_t function;
}AT_Command_ApiMode_t;

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



uint8 calCheckSum(uint8 *in, int len);
uint16 assembleApiFrame(tsApiFrame *frm, teFrameType type, uint8 *payload, uint16 payloadLen);
uint16 deassembleApiFrame(uint8 *buffer, int len, tsApiFrame *frm, bool *valid);
uint16 u16DecodeApiSpec(uint8 *buffer, int len, tsApiSpec *spec, bool *valid);		//oliver add for API mode
void copyApiFrame(tsApiFrame *frm, uint8 *dst);
bool searchAtStarter(uint8 *buffer, int len);
int processSerialCmd(uint8 *buf, int len);
int ProcessApiCmd(tsApiSpec* apiSpec);

#endif /* __AT_API_H__ */
