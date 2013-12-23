/*    
 * node.h
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

#ifndef __NODE_H__
#define __NODE_H__




/* address of the IPD. In the field this address will be added to
 * the table out of band, not hard coded
 */
#define IPD_MAC_ADDR1             0x0000000000000001LL
#define IPD_MAC_ADDR2             0x0000000000000002LL
#define IPD_MAC_ADDR3             0x0000000000000003LL
#define IPD_MAC_ADDR4             0x0000000000000004LL
#define IPD_MAC_ADDR5             0x0000000000000005LL



/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define ESP_METER_LOCAL_EP         1
#define MIRROR_START_EP            2

#define RESTART_TIME            APP_TIME_MS(1000)

#define ONE_SECOND_TICK_TIME    APP_TIME_MS(1000)

#define EMPTY                    0
#define ONE_MINUTE                1
#define TWO_MINUTES                2
#define ONE_HOUR                60
#define MAX_TIME_INTERVAL         65535

//#define RADIO_RECALIBRATION

#define PDM_REC_MAGIC            0x55667788
#define REC_ID1                    0x1


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void node_vInitialise(void);
PUBLIC void deleteStackPDM();

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*__NODE_H__*/
