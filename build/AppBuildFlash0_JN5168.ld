/*    
 * AppBuildFlash0_JN5168.ld
 * Firmware for SeeedStudio Mesh_Bee(Zigbee) module 
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

INCLUDE BootLoaderFunctions.ld
INCLUDE App_Stack_Size.ld
INCLUDE FixedLocationBootFunctions.ld

EXTERN(_AppWarmStart)
EXTERN(_AppColdStart)


_SwConfig =  (DEFINED(g_bSWConf_Debug)) | (DEFINED(g_bSWConf_AltDebugPort)<<1);

MEMORY
{
        flash  : ORIGIN = 0x00080000, LENGTH = 0x40000
        ram    : ORIGIN = 0x0400004c, LENGTH = 0x07fb4
}

/* Now building all code at once, so include vectors. MAC address is embedded
   in build for now */
SECTIONS
{
        .version ABSOLUTE(ORIGIN(flash)-4):
        {
        /*
        0008 - Chip Type 6x
        03   - 32K Ram
        07   - 256K Flash
            */
        LONG(0x07030008)
        } > flash 
                       
        .bir ABSOLUTE(ORIGIN(flash)):
        {
          _flash_start = ABSOLUTE(.);
          _flash_beg = ABSOLUTE(.);
        /* Magic number */
        LONG(0x12345678)
        LONG(0x11223344)
        LONG(0x55667788)
        
        /* Configuration A (32K, DRE, 16MHz) */
        BYTE(0x08)

        /* Configuration B (version 1) */
        BYTE(0x01)
        
        /* Application ID */
        SHORT(0x0000)
        } > flash
        
        .flashheader :
        {
        /* Encryption IV */
        LONG(0x00000000)
        LONG(0x00000000)
        LONG(0x00000000)
        SHORT(0x0000)

        /*SW Config*/
        /*
        Bit
        0    Debug En (1 Enable 0 Disable)
        1    0 - JTAG Debug on DIO4-7
             1 - JTAG Debug on DIO12-15
        2-15 Unused
            */
        SHORT(ABSOLUTE(_SwConfig))

        _enc_start = ABSOLUTE(.);
        /*Flash Image Length*/
        LONG(ABSOLUTE(_image_len))
        _enc_offset = (_enc_start & 0x0000000F);
        _enc_start = _enc_start  - _flash_beg;

        /* Data start & length */
        LONG(ABSOLUTE(_flash_data_start))
        SHORT((ABSOLUTE(_ram_data_start) / 4) & 0xffff)
        SHORT((ABSOLUTE(_ram_data_len)   / 4) & 0xffff)

        /* BSS start & length */
        SHORT((ABSOLUTE(_bss_start) / 4) & 0xffff)
        SHORT((ABSOLUTE(_bss_len)   / 4) & 0xffff)

        /* Entry points */
        LONG(ABSOLUTE(_AppWarmStart))
        LONG(ABSOLUTE(_AppColdStart))
        } > flash

}
