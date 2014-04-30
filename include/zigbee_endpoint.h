/*
 * zigbee_endpoint.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log : Oliver Wang Modify 2014/3
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

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

#include <jendefs.h>
#include "zps_apl_aib.h"
#include "firmware_at_api.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define TRANS_CLUSTER_ID                0x1000
#define TRANS_ENDPOINT_ID               (1)
#define DATA_POINT_NUM					1
#define VERIFY_BYTE						0x3e

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

//datastream:A series of data points
typedef struct
{
	uint8 verifyByte;			     	//verify,avoiding mistakes when take serial transmission
	uint8 dataType;					 	// maybe on-chip temperature or voltage
	uint16 dataPointCnt;			 	// number of data point
	int16 datapoint[DATA_POINT_NUM];	//a series of data points
}tsDataStream;

typedef enum
{
	INNER_TEMP = 1,
	INNER_VOL = 2
	/*your own data type here*/
}teDataType;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void clientOtaFinishing();
bool sendToAir(uint16 txmode, uint16 unicastDest, tsApiFrame *apiFrame,  teFrameType type, uint8 *buff, int len);
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /* __ENDPOINT_H__ */
