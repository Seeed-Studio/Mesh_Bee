/*    
 * app_ota.h
 * Firmware for SeeedStudio RFBeeV2(Zigbee) module 
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

#ifndef __OTA_H__
#define __OTA_H__

#include "common.h"

#define OTA_BLOCK_SIZE              50
#define OTA_SECTOR_SIZE             64*1024
#define OTA_SECTOR_CNT              8
#define OTA_MAGIC_OFFSET            0x0
#define OTA_IMAGE_LEN_OFFSET        0x20
#define OTA_MAGIC_NUM_LEN           12

extern uint8 magicNum[OTA_MAGIC_NUM_LEN];

PUBLIC void APP_vOtaFlashLockRead(uint32 offsetByte, uint16 len, uint8 *dest);
PUBLIC void APP_vOtaFlashLockWrite(uint32 offsetByte, uint16 len, uint8 *buff);
PUBLIC void APP_vOtaFlashLockErase(uint8 sector); 
PUBLIC void APP_vOtaFlashLockEraseAll(); 
PUBLIC void APP_vOtaKillInternalReboot();

PUBLIC void init_crc_table(void);
PUBLIC unsigned int crc32(unsigned int crc, unsigned char *buffer, unsigned int size); 
PUBLIC uint32 imageCrc(uint32 imageLen); 


#endif /* __OTA_H__ */
