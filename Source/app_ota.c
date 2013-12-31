/*    
 * app_ota.c
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


#include "common.h"
#include "app_ota.h"


#ifndef TRACE_OTA
#define TRACE_OTA TRUE
#endif


uint8 magicNum[OTA_MAGIC_NUM_LEN] = { 0x12, 0x34, 0x56, 0x78, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 }; // TODO fill the magic value

static unsigned int crc_table[256];

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void APP_vOtaFlashLockRead(uint32 offsetByte, uint16 len, uint8 *dest)
{
    OS_eEnterCriticalSection(hSpiMutex);
    bAHI_FullFlashRead(offsetByte, len, dest);
    OS_eExitCriticalSection(hSpiMutex);
}

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void APP_vOtaFlashLockWrite(uint32 offsetByte, uint16 len, uint8 *buff)
{
    OS_eEnterCriticalSection(hSpiMutex);
    bAHI_FullFlashProgram(offsetByte, len, buff);
    OS_eExitCriticalSection(hSpiMutex);
}

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void APP_vOtaFlashLockErase(uint8 sector)
{
    OS_eEnterCriticalSection(hSpiMutex);
    bAHI_FlashEraseSector(sector);
    OS_eExitCriticalSection(hSpiMutex);
}

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void APP_vOtaFlashLockEraseAll()
{
    int i = 0;
    for (i = 0; i < OTA_SECTOR_CNT; i++)
    {
        APP_vOtaFlashLockErase(i);
    }
}

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void APP_vOtaKillInternalReboot()
{

    //put internal flash invalid
    uint8 au8Data[16] = { 0 };

    // Invalidate Internal Flash Header
    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL); /* Pass Internal Flash */
    // Erase Internal Flash Header
    bAHI_FullFlashProgram(0x00, 16, au8Data);

    //trigger reboot
    DBG_vPrintf(1, "Now reboot into upgrading...\r\n");
    vAHI_SwReset();
}


/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;
    
    for (i = 0; i < 256; i++)
    {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++)
        {
            if (c & 1)  c = 0xedb88320L ^ (c >> 1);
            else  c = c >> 1;
        }
        crc_table[i] = c;
    }
}


/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC unsigned int crc32(unsigned int crc, unsigned char *buffer, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

/****************************************************************************
 *
 * NAME: function below
 *
 * DESCRIPTION:
 * 
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
PUBLIC uint32 imageCrc(uint32 imageLen)
{
    uint32 i = 0;
    uint32 rdLen = 0;
    uint8 buff[128];
    uint32 crc = 0xffffffff;

    init_crc_table();

    for (i = 0; i < imageLen; i += 128)
    {
        rdLen = (imageLen - i) >= 128 ? 128 : ((imageLen - i));
        APP_vOtaFlashLockRead(i, rdLen, buff);
        crc = crc32(crc, buff, rdLen);
    }
    return crc;
}
