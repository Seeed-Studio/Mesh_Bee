/*    
 * AppBuildFlash1_JN5168.ld
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

/* Now building all code at once, so include vectors. MAC address is embedded
   in build for now */
SECTIONS
{
        
    /* start location 0x54 */
    .ro_mac_address (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
    _FlsMACAddress = ABSOLUTE(.);
    *(.ro_mac_address) /* Mac Address 8 bytes*/
    . = ALIGN (0x10);
    } > flash        
    
    .ro_ota_header (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
    _FlsOtaHeader = ABSOLUTE(.);
    *(.ro_ota_header) /* Ota Header 69 bytes*/
    . = ALIGN (0x10);
    } > flash   
    
    .ro_se_lnkKey (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
    _FlsLinkKey = ABSOLUTE(.);
    *(.ro_se_lnkKey)  /* Link Key 16 bytes*/
    . = ALIGN (0x10);
        } > flash

    .ro_se_cert (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
    _FlsZcCert = ABSOLUTE(.);
    *(.ro_se_cert)  /* Zigbee certificate 48 bytes*/
    . = ALIGN (0x10);
        } > flash

    .ro_se_pvKey  (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
    _FlsPrivateKey = ABSOLUTE(.);
    *(.ro_se_pvKey) /* Private Key 21 Bytes */
        . = ALIGN (0x10);
        } > flash
        
    .ro_se_customData (ALIGN((. - _enc_offset), 16) + _enc_offset) :
    {
    _FlsCustomDatabeg = ABSOLUTE(.);
    *(.ro_se_customData) /* Custom Data */
        . = ALIGN (0x10);
        } > flash    
        _FlsCustomDataEnd = ABSOLUTE(.);    

}
