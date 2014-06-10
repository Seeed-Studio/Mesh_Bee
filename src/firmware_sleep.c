/*
 * firmware_sleep.c
 * Handles sleep mode of End Device
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2014/4
 * Change Log : [Oliver: Modify 2014/05]
 *              [Oliver: Modify 2014/05] Poll period set to 200ms
 *              [Oliver: Modify 2014/06] DATA/API mode support for sleep too
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
#include "firmware_aups.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_SLEEP
#define TRACE_SLEEP FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/* wake up call back */
PUBLIC void vWakeCallBack(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
static bool SLEEP_ENABLE = false;  /* if enable a sleep, set this flag */

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32 _wakeupTime = 0;

/* Pointer to a structure to be populated with the wake point and callback function */
PRIVATE	pwrm_tsWakeTimerEvent	sWake;

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: SleepScheduleTask
 *
 * DESCRIPTION:
 * Sleep schedule task
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(SleepScheduleTask)
{
#ifdef TARGET_END
	    Sleep(g_sDevice.config.sleepPeriod);
#endif
}

/****************************************************************************
 *
 * NAME: vSleepSchedule
 *
 * DESCRIPTION:
 * Sleep schedule main entry
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSleepSchedule(void)
{
	/* Only the end device which works in DIO and Timer mode can call this function */
	if(SLEEP_MODE_WAKE_BY_TIMER == g_sDevice.config.sleepMode ||
			SLEEP_MODE_WAKE_BY_DIO_TIMER == g_sDevice.config.sleepMode)
	{
		vResetATimer(SleepTimer, APP_TIME_MS(g_sDevice.config.sleepWaitingTime));
	}
}
/****************************************************************************
 *
 * NAME: Sleep
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
PUBLIC void Sleep(uint32 ms)
{
#ifdef TARGET_END
	DBG_vPrintf(TRACE_SLEEP, "Sleep %ld ms\r\n", ms);

	/* Set the next wake point */
	_wakeupTime = ms;
	PWRM_eScheduleActivity(&sWake, _wakeupTime*32 , vWakeCallBack);

	/*
	 * Stop all software timers, prepare for sleeping
	 * Must stop all of the swTimers, otherwise can not enter sleep mode again
    */
    stopAllSwTimers();

    /* Set this flag */
    SLEEP_ENABLE = true;
#endif
}

/****************************************************************************
 *
 * NAME: sleep
 *
 * DESCRIPTION:
 * set the End Device into sleep mode for n s
 *
 * PARAMETERS: Name         RW  Usage
 *             s            W   s
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void sleep(uint16 s)
{
    uint32 ms = s*1000;
    Sleep(ms);
}

/****************************************************************************
 *
 * NAME: usleep
 *
 * DESCRIPTION:
 * set the End Device into sleep mode for n us
 *
 * PARAMETERS: Name         RW  Usage
 *             us           W   us
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void usleep(uint32 us)
{
    uint32 ms = us/1000;
    Sleep(ms);
}

/****************************************************************************
 *
 * NAME: bGetSleepStatus
 *
 * DESCRIPTION:
 * determine a sleep enabled or not
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * status
 *
 ****************************************************************************/
PUBLIC bool bGetSleepStatus(void)
{
    return SLEEP_ENABLE;
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
	/*
	 * Cannot schedule the next wake event until the wake up callback
	 * function has completed. Instead, activate a high priority task
	 * so that it runs immediately after the callback
	*/
	OS_eActivateTask(WakeUpTask);
}

/****************************************************************************
 *
 * NAME: APP_WakeUpTask
 *
 * DESCRIPTION:
 * Wake up initialization task
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(WakeUpTask)
{
    DBG_vPrintf(TRACE_SLEEP, "WakeUp Task.\r\n");

    /* Clean sleep flag */
    SLEEP_ENABLE = false;

    /* When a device wakes up, Poll from its parent immediately */
    OS_eActivateTask(PollTask);

    /* Wakeup houseKeepping task: determine state */
    if(E_MODE_MCU == g_sDevice.eMode)
    {
    	ups_init();        //Initialize AUPS
    }
    else if(E_MODE_AT == g_sDevice.eMode)
    {
    	//do nothing now
    }
    else
    {
    	vSleepSchedule(); //Reset SleepScheduleTimer again in DATA/API mode
    }
}
/****************************************************************************
 *
 * NAME: PollTask
 *
 * DESCRIPTION:
 * Polling Task requests the buffered data and should normally be
 * called immediately after waking from sleep.
 *
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

    /* Poll per 200ms */
    OS_eStartSWTimer(PollTimer, APP_TIME_MS(200), NULL);
}


/****************************************************************************
 *
 * NAME: stopAllSwTimers
 *
 * DESCRIPTION:
 * stop all of the software timers, prepare for a sleeping
 *
 * PARAMETERS: Name         RW  Usage
 *
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
    if(OS_eGetSWTimerStatus(PollTimer) != OS_E_SWTIMER_STOPPED)
    {
    	OS_eStopSWTimer(PollTimer);
    }
    if(OS_eGetSWTimerStatus(SleepTimer) != OS_E_SWTIMER_STOPPED)
    {
    	OS_eStopSWTimer(SleepTimer);
    }
#endif
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
