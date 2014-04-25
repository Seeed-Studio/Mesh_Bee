/*
 * firmware_api_codec.c
 * API codec library for MCU mode(arduino-ful MCU mode)
 *
 * Copyright (c) Seeed Studio. 2014.
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
#include "firmware_api_pack.h"
#include "firmware_at_api.h"

/****************************************************************************/
/***        External Functions                                            ***/
/****************************************************************************/
extern uint8 calCheckSum(uint8 *in, int len);

/****************************************************************************
 *
 * NAME: u16DecodeApiSpec
 *
 * DESCRIPTION:
 * length = cmdData  [delimiter length apiIdentifier cmdData checkSum]
 * Pay attention: 4 bytes align
 * RETURNS:
 * position of start delimiter
 *
 ****************************************************************************/
uint16 u16DecodeApiSpec(uint8 *buffer, int len, tsApiSpec *spec, bool *valid)
{
	uint8 *ptr = buffer;
	while (*ptr != API_START_DELIMITER && len-- > 0) ptr++;
	if (len < 4)
	{
	    *valid = FALSE;
	    return ptr - buffer;
	}

	/* read startDelimiter/length/apiIdentifier */
	memcpy((uint8*)spec, ptr, 3);    //1 bytes align,read 3 bytes

    ptr += 3;
    len -= 3;
    if (len < (spec->length + 1))
    {
        *valid = FALSE;
        return ptr - buffer;
    }

    /* read payload */
    memcpy((uint8*)spec + 3, ptr, spec->length);
    ptr += spec->length;

    /* read checkSum,redundant bytes aren't transfered */
    spec->checkSum = *ptr;
    ptr++;

    /* verify checkSum */
    if (calCheckSum((uint8*)spec+3,spec->length) == spec->checkSum)
    {
        *valid = TRUE;
    }
    else
    {
        *valid = FALSE;
    }
    return ptr-buffer;
}

/****************************************************************************
 *
 * NAME: vCopyApiSpec
 *
 * DESCRIPTION:
 * Copy the valid bytes of tsApiSpec into dst
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
int vCopyApiSpec(tsApiSpec *spec, uint8 *dst)
{
	int size = 0;
    memcpy(dst, (uint8 * )spec, 3);
    dst += 3;
    size += 3;

    memcpy(dst, &(spec->payload), spec->length);
    dst += spec->length;
    size += spec->length;

    memcpy(dst, &(spec->checkSum), 1);
    size += 1;

    return size;
}

/****************************************************************************
 *
 * NAME: PAK_vApiSpecDataFrame
 *
 * DESCRIPTION:
 * Pack data frame
 *
 * RETURNS:
 * tsApiSpec
 *
 ****************************************************************************/
void PCK_vApiSpecDataFrame(tsApiSpec *apiSpec, uint8 frameId, uint8 option, uint16 unicastAddr, void *data, int len)
{
	tsTxDataPacket txDataPacket;
    memset(&txDataPacket, 0, sizeof(tsTxDataPacket));

    int min_cnt = MIN(len, API_DATA_LEN);

	txDataPacket.frameId = frameId;
    txDataPacket.option = option;
    txDataPacket.dataLen = min_cnt;
    txDataPacket.unicastAddr = unicastAddr;

    memcpy(txDataPacket.data, data, min_cnt);

    apiSpec->startDelimiter = API_START_DELIMITER;
    apiSpec->length = sizeof(tsTxDataPacket);
    apiSpec->teApiIdentifier = API_DATA_PACKET;
    apiSpec->payload.txDataPacket = txDataPacket;
    apiSpec->checkSum = calCheckSum((uint8*)&txDataPacket, apiSpec->length);
}


uint8 PCK_u8ApiSpecLocalAtIo(tsApiSpec *apiSpec, uint8 pin, uint8 state)
{
  apiSpec->startDelimiter = API_START_DELIMITER;
  apiSpec->length = sizeof(tsLocalAtReq);           //Note: union length != tsLocalAtReq length
  apiSpec->teApiIdentifier = API_LOCAL_AT_REQ;

  tsLocalAtReq localAtReq;
  memset(&localAtReq, 0, sizeof(tsLocalAtReq));

  localAtReq.frameId = 0xec;
  localAtReq.atCmdId = ATIO;
  localAtReq.value[0] = pin;
  localAtReq.value[1] = state;

  apiSpec->payload.localAtReq = localAtReq;

  apiSpec->checkSum = calCheckSum((uint8*)&localAtReq, apiSpec->length);

  return 3 + sizeof(tsLocalAtReq) + 1;
}

uint8 PCK_u8ApiSpecRemoteAtIo(tsApiSpec *apiSpec, uint16 unicastAddr , uint8 pin, uint8 state)
{
  apiSpec->startDelimiter = API_START_DELIMITER;
  apiSpec->length = sizeof(tsRemoteAtReq);           //Note: union length != tsLocalAtReq length
  apiSpec->teApiIdentifier = API_REMOTE_AT_REQ;

  tsRemoteAtReq remoteAtReq;
  memset(&remoteAtReq, 0, sizeof(tsRemoteAtReq));

  remoteAtReq.frameId=0xec;
  remoteAtReq.atCmdId = ATIO;
  remoteAtReq.option = 0x00;
  remoteAtReq.value[0] = pin;
  remoteAtReq.value[1] = state;
  remoteAtReq.unicastAddr = unicastAddr;

  apiSpec->payload.remoteAtReq = remoteAtReq;
  apiSpec->checkSum = calCheckSum((uint8*)&remoteAtReq, apiSpec->length);

  return 3 + sizeof(tsRemoteAtReq) + 1;
}
