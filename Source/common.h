/*    
 * common.h
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

#ifndef GLOBAL_DEF_H_
#define GLOBAL_DEF_H_

#include <jendefs.h>
#include "dbg.h"
#include "dbg_uart.h"
#include "os.h"
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "AppHardwareAPI.h"
#include "pdm.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"

#include "os_gen.h"
#include "pdum_gen.h"
#include "zps_gen.h"


#include "string.h"
#include "app_timer_driver.h"
#include "app_zbp_utilities.h"
#include "Time.h"
#include "Utilities.h"
#include "Printf.h"

#include "ringbuffer.h"

#define SW_VER                          0x1000

#if defined(TARGET_COO) || defined(TARGET_ROU) || defined(TARGET_END)
#else
    #define TARGET_COO
#endif

#define RADIO_RECALIBRATION                                  //re-calibrate the radio per 1min
#define SEC_MODE_FOR_DATA_ON_AIR    ZPS_E_APL_AF_SECURE_NWK    //securing mode for the packets passing through the air
                                                             //ZPS_E_APL_AF_UNSECURE or ZPS_E_APL_AF_SECURE_NWK


#ifdef OTA_SUPPORT_OPTIONS        // Option flag passed in from the makefile
#define CLD_OTA
#ifdef TARGET_COO
#define OTA_SERVER
#else
#define OTA_CLIENT
#endif
#endif


#define UART_COMM                       E_AHI_UART_1
#define MAX_ROUTE_DISCOVERY_FAILURES    10
#define DIO_ON_SLEEP                    9
#define DIO_ASSOC                       10
#define DIO_RSSI                        11

#define true                            1
#define false                           0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_NETWORK_CONFIG,
    E_NETWORK_STARTUP,
    E_NETWORK_WAIT_FORMATION,
    E_NETWORK_NFN_START,
    E_NETWORK_DISCOVERY,
    E_NETWORK_JOINING,
    E_NETWORK_INIT,
    E_NETWORK_RESCAN,
    E_NETWORK_WAIT_LEAVE,
    E_NETWORK_RUN

} teState;

typedef enum
{
    E_SUB_NONE,
    E_SUB_RESCANNING,
    E_SUB_REJOINNING
}teSubState; 

typedef enum
{
    E_MODE_DATA,
    E_MODE_AT,
    E_MODE_API
}teMode;

enum teTxMode
{
    BROADCAST,
    UNICAST
};

typedef struct
{
    uint16             powerupApi;
    uint16             powerUpAction;
    uint16             autoJoinFirst;
    uint16             networkToJoin;
    uint16             txMode;
    uint16             unicastDstAddr;
    uint16             baudRateUart1;
    uint16             sleepMode;
    uint16             wakeupDuration;
    uint16             reqPeriodMs;
}tsConfig; 


typedef struct
{
    uint32      magic;
    uint32      len;
    teState     eState;
    teSubState  eSubState;
    teMode      eMode;
    tsConfig    config;
    ZPS_tsNwkNetworkDescr   nwDesc;
    //OTA related
    #ifdef CLD_OTA
    bool        supportOTA;
    bool        isOTASvr; 
    uint32      otaTotalBytes; 
    uint32      otaTotalBlocks;
    uint32      otaReqPeriod;
    uint32      otaCurBlock;
    uint16      otaSvrAddr16;
    uint8       otaDownloading;  //0: idle; 1: block downloading; 2: upgrade requesting
    uint32      otaCrc;
    #endif
} tsDevice; 


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern struct ringbuffer rb_rx_uart;
extern struct ringbuffer rb_tx_uart; 
extern tsDevice g_sDevice; 
extern PDM_tsRecordDescriptor g_sDevicePDDesc;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vResetATimer(OS_thSWTimer hSWTimer, uint32 u32Ticks);

#endif /* GLOBAL_DEF_H_ */
