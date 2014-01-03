/*    
 * at_api.h
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

#ifndef __AT_API_H__
#define __AT_API_H__

#include <jendefs.h>
#include "uart.h"
#include "app_ota.h"

typedef enum
{
    FRM_CTRL,
    FRM_QUERY,
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

//API
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

//AT
typedef int (*AT_Command_Function_t)(uint16 *); 

typedef int8 byte;

typedef struct
{
    const char    *name;
    uint16        *configAddr;   // the ID used in the EEPROM
    const int     paramDigits;  // how many digits for the parameter
    const uint16  maxValue;     // maximum value of the parameter
    const bool    postProcess;  // do we need to call the function to perform extra actions on change
    AT_Command_Function_t function; // the function which does the real work on change
    const bool    reboot;
}  AT_Command_t; 

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
void copyApiFrame(tsApiFrame *frm, uint8 *dst);
bool searchAtStarter(uint8 *buffer, int len);
int processSerialCmd(uint8 *buf, int len);

#endif /* __AT_API_H__ */
