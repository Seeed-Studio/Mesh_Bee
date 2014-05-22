/*
 * firmware_cmi.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
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

#ifndef FIRMWARE_CMI_H_
#define FIRMWARE_CMI_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "common.h"
#include "firmware_at_api.h"
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void CMI_vPushData(void *data, int len);
//void CMI_vTxData(void *data, int len);
PUBLIC void CMI_vAirDataDistributor(tsApiSpec *apiSpec);
PUBLIC void CMI_vUrtRevDataDistributor(void *data, int len);
PUBLIC void CMI_vUrtAckDistributor(tsApiSpec *apiSpec);
#endif /* FIRMWARE_CMI_H_ */

