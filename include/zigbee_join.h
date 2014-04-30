/*
 * zigbee_join.h
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

#ifndef ZIGBEE_JOIN_H_
#define ZIGBEE_JOIN_H_


#include "common.h"
#include "firmware_at_api.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vHandleStartupEvent(void);
PUBLIC void vHandleNetworkLeave(ZPS_tsAfEvent sStackEvent);
PUBLIC void vHandleNetworkDiscoveryEvent(ZPS_tsAfEvent sStackEvent);
PUBLIC void vHandleNetworkJoinEvent(ZPS_tsAfEvent sStackEvent);
PUBLIC void vJoinStoredNWK(void);
PUBLIC void vSaveDiscoveredNWKS(ZPS_tsAfEvent sStackEvent);
PUBLIC void vDisplayDiscoveredNWKS(void);
PUBLIC void vUnhandledEvent(uint8 eState, ZPS_teAfEventType eType);
PUBLIC void vStartStopTimer( OS_thSWTimer hSWTimer, uint32 u32Ticks, teState eNextState );
PUBLIC void vJoinedNetwork( ZPS_tsAfEvent sStackEvent );
PUBLIC void vRestoreLastNWK(ZPS_tsNwkNetworkDescr * desc);

int AT_listNetworkScaned(uint16 *regAddr);
int AT_reScanNetwork(uint16 *regAddr);
int AT_joinNetworkWithIndex(uint16 *regAddr);
int API_RescanNetwork_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_JoinNetworkWithIndex_CallBack(tsApiSpec *inputApiSpec, tsApiSpec *retApiSpec, uint16 *regAddr);
int API_listNetworkScaned_CallBack(tsApiSpec *reqApiSpec, tsApiSpec *respApiSpec, uint16 *regAddr);

#define MAX_SINGLE_CHANNEL_NETWORKS        8

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern     bool                    bRejoining;
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*ZIGBEE_JOIN_H_*/
