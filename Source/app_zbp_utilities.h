/*    
 * app_zbp_utilities.h
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

#ifndef APP_ZBP_UTILITIES_H_
#define APP_ZBP_UTILITIES_H_


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void vClearDiscNT(void);
PUBLIC void vRemoveCoordParents(void);
PUBLIC void vDisplayDiscNT(void);
PUBLIC bool bInRoutingTable(uint16 u16ShortAddress);
PUBLIC void vDisplayRoutingTable(void);
PUBLIC void vDisplayTableSizes(void);
PUBLIC void vDisplayAddressMapTable(void);
PUBLIC void vDisplayNT(void);
PUBLIC void vClearRoutingTable(void);
PUBLIC void vDisplayRouteRecordTable(void);
PUBLIC void vClearRouteRecordTable(void);PUBLIC void vDisplayAPSTable(void);
PUBLIC void vDisplayNWKTransmitTable(void);

#ifdef TRACE_TIMERS
PUBLIC void vDebugTimers(void);
#endif

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        In line  Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_ZBP_UTILITIES_H_*/
