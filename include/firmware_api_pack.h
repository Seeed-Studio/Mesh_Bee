/*
 * firmware_api_codec.h
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

#ifndef FIRMWARE_API_CODEC_H_
#define FIRMWARE_API_CODEC_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "firmware_at_api.h"




/****************************************************************************/
/***        Public Functions                                              ***/
/****************************************************************************/
uint16 u16DecodeApiSpec(uint8 *buffer, int len, tsApiSpec *spec, bool *valid);
int i32CopyApiSpec(tsApiSpec *spec, uint8 *dst);

PUBLIC void PCK_vApiSpecDataFrame(tsApiSpec *apiSpec, uint8 frameId, uint8 option, uint16 unicastAddr, void *data, int len);
PUBLIC uint8 PCK_u8ApiSpecLocalAtIo(tsApiSpec *apiSpec, uint8 pin, uint8 state);
PUBLIC uint8 PCK_u8ApiSpecRemoteAtIo(tsApiSpec *apiSpec, uint16 unicastAddr , uint8 pin, uint8 state);
#endif /* FIRMWARE_API_CODEC_H_ */
