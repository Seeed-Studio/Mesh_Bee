/*
 * firmware_sleep.c
 * Handles sleep mode of End Device
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 * 
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2014/4
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "common.h"
#include "firmware_sleep.h"
#include "firmware_ups.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_SLEEP
#define TRACE_SLEEP TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PUBLIC void vWakeCallBack(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32 _wakeupTime = 0;
PRIVATE	pwrm_tsWakeTimerEvent	sWake;

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: goSleepMs
 *
 * DESCRIPTION:
 * set the End Device into sleep mode for n ms
 *
 * PARAMETERS: Name         RW  Usage
 *             ms           W   ms
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void goSleepMs(uint32 ms)
{
#ifdef TARGET_END
    _wakeupTime = ms;
    OS_eActivateTask(SleepEnableTask);
#endif
}


/****************************************************************************
 *
 * NAME: SleepEnableTask
 *
 * DESCRIPTION:
 * task for enabling sleep mode
 * runs at lowest priority to ensure all other tasks are inactive
 *
 ****************************************************************************/
OS_TASK(SleepEnableTask)
{
#ifdef TARGET_END
    DBG_vPrintf(TRACE_SLEEP, "SleepEnable Task\r\n"); 
    
    PWRM_eScheduleActivity(&sWake, _wakeupTime*32 , vWakeCallBack);		// Schedule the next sleep point
    
    stopAllSwTimers();
#endif
}


/****************************************************************************
 *
 * NAME: vWakeCallBack
 *
 * DESCRIPTION:
 * Passed to the schedule activity, and then called by the PWRM on wake
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vWakeCallBack(void)
{
	// Cannot schedule the next wake event until the wake up callback
	// function has completed. Instead, activate a high priority task
	// so that it runs immediately after the callback
	OS_eActivateTask(WakeUpTask);
}

/****************************************************************************
 *
 * NAME: APP_WakeUpTask
 *
 * DESCRIPTION:
 * Wakeup initialisation task
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(WakeUpTask)
{
    DBG_vPrintf(TRACE_SLEEP, "Wake up task\r\n");
    
    OS_eActivateTask(PollTask); 

    ups_init(); 
    
}

/****************************************************************************
 *
 * NAME: APP_WakeUpTask
 *
 * DESCRIPTION:
 * Wakeup initialisation task
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(PollTask)
{
    DBG_vPrintf(TRACE_SLEEP, "Poll task\r\n");

    ZPS_teStatus u8PStatus = ZPS_eAplZdoPoll();

    if (u8PStatus)
    {
        DBG_vPrintf(TRACE_SLEEP, "\nPoll Failed %d\n", u8PStatus);
    }
    
    //OS_eStartSWTimer(PollTimer, APP_TIME_MS(1), NULL); 
}


/****************************************************************************
 *
 * NAME: stopAllSwTimers
 *
 * DESCRIPTION:
 * set the End Device into sleep mode after n ms
 *
 * PARAMETERS: Name         RW  Usage
 *             ms           W   ms
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void stopAllSwTimers()
{
#ifdef TARGET_END
    if (OS_eGetSWTimerStatus(App_tmr1sec) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(App_tmr1sec);
    }
    if (OS_eGetSWTimerStatus(APP_RouteRequestTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_RouteRequestTimer);
    }
    if (OS_eGetSWTimerStatus(APP_JoinTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_JoinTimer);
    }
    if (OS_eGetSWTimerStatus(APP_OTAReqTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_OTAReqTimer);
    }
    if (OS_eGetSWTimerStatus(APP_tmrHandleUartRx) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_tmrHandleUartRx);
    }
    if (OS_eGetSWTimerStatus(APP_AgeOutChildrenTmr) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_AgeOutChildrenTmr);
    }
    if (OS_eGetSWTimerStatus(APP_RadioRecalTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_RadioRecalTimer);
    }
    if (OS_eGetSWTimerStatus(APP_RejoinTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_RejoinTimer);
    }
    if (OS_eGetSWTimerStatus(Arduino_LoopTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(Arduino_LoopTimer);
    }
#endif
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

