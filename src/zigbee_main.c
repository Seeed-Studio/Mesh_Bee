/*
 * zigbee_main.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log : Oliver Wang Modify 2014/03
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
#include "appapi.h"
#include "zigbee_node.h"
#include "firmware_uart.h"

#include "firmware_at_api.h"
#include "firmware_hal.h"

#include "suli.h"

#ifdef RADIO_RECALIBRATION
#include "recal.h"
#endif

#ifdef PDM_EEPROM
PUBLIC uint8 u8PDM_CalculateFileSystemCapacity(void);
PUBLIC uint8 u8PDM_GetFileSystemOccupancy(void);
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_START
#define TRACE_START       FALSE
#endif

#ifndef TRACE_OVERLAYS
#define TRACE_OVERLAYS    FALSE
#endif

#ifndef TRACE_EXCEPTION
#define TRACE_EXCEPTION    TRUE
#endif

#define RAM_HELD    2

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vInitialiseApp(void);
PRIVATE void vUnclaimedInterrupt(void);
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject);
PRIVATE void vSetUpWakeUpConditions(void);

#ifdef PDM_EEPROM
PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode);
#endif
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(PreSleep);
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(Wakeup);

/* encryption key for PDM */
PRIVATE const tsReg128 g_sKey = { 0x45FDF4C9, 0xAE9A6214, 0x7B27285B, 0xDB7E4557 };

/* On/Sleep Led */
extern IO_T SleepLed;
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

#if defined(PRODUCTION) || defined(CLD_OTA)
extern uint16 u16ImageStartSector;
#else
uint16 u16ImageStartSector = 0;
#endif

/* Linker script extern */
extern void *stack_low_water_mark;
extern void *stack_size;
extern void *free_ram_len;

/****************************************************************************/
/***        Tasks                                                          ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

  /*
   * The Power Manager is initialized and started using the function PWRM_vInit()
   * here, the 32-kHz oscillator and memory are left running during sleep,
   * PWRM_vScheduleActivity() can schedule the wake point,refer to firmware_sleep.c
  */

/****************************************************************************
 *
 * NAME: PreSleep
 *
 * DESCRIPTION:
 * Will be called before sleep
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
PWRM_CALLBACK(PreSleep)
{
    DBG_vPrintf(TRACE_START, "APP: Going to sleep (CB) ... ");

    /* Turn off On/Sleep Led */
    suli_pin_write(&SleepLed, HAL_PIN_LOW);

    /* Turn radio off */
    vAppApiSaveMacSettings();
    vAHI_AdcDisable();
	vAHI_SiMasterDisable();

	/* Clear DIO interrupts */
    u32AHI_DioInterruptStatus();
    DBG_vPrintf(TRACE_START, "gone\n\n");

    /* Disable UART */
    vAHI_UartDisable(E_AHI_UART_0);

    /* if wake by DIO, Set up wake up input */
    if(SLEEP_MODE_WAKE_BY_DIO_TIMER == g_sDevice.config.sleepMode)
    {
        vSetUpWakeUpConditions();
    }
}

/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 * Will be called after waking up in any mode
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
PWRM_CALLBACK(Wakeup)
{
	/* Make sure the clock source is the external 32MHz XTAL, required for UART timings */
    while (bAHI_GetClkSource() == TRUE);

    /*
	  This function recalculates the wait-state settings for the internal Flash memory and
	  EEPROM devices after the system clock source or CPU  clock frequency has been
	  changed to minimise the Flash access time.
	*/
    vAHI_OptimiseWaitStates();

    /* Need to reInitialise the SPI bus for external PDM */
#ifndef PDM_EEPROM
    PDM_vWarmInitHW();
#endif

    /* Init UART0 */
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

    DBG_vPrintf(TRACE_START, "\r\n\r\nAPP: Woken up (CB)\r\n");

    if( (u8AHI_PowerStatus()) & RAM_HELD )
    {
		/* Restore Mac settings (turns radio on) */
		vMAC_RestoreSettings();
		DBG_vPrintf(TRACE_START, "APP: MAC settings restored\n");

		vAHI_HighPowerModuleEnable(TRUE, TRUE);

		/* Limit the module to +8dB for ETSI compliance */
#ifdef ETSI
		vAHI_ETSIHighPowerModuleEnable(TRUE);
#endif

		/* Re-initialise the hardware peripherals */
        uart_initialize();

        /* Init ADC */
        tsAdcParam tsParm;
        tsParm.u8SampleSelect = E_AHI_AP_SAMPLE_8;
        tsParm.u8ClockDivRatio = E_AHI_AP_CLOCKDIV_500KHZ;
        tsParm.bRefSelect = E_AHI_AP_INTREF;

        vHAL_AdcSampleInit(&tsParm);

        /* Init PWM for RSSI Led */
        vAHI_TimerEnable(E_AHI_TIMER_1, 4, FALSE, FALSE, TRUE);
        vAHI_TimerStartRepeat(E_AHI_TIMER_1, 1000, 1);

        bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);

        /* Restart the RTOS */
        OS_vRestart();
        DBG_vPrintf(TRACE_START, "APP: Restarting OS\n");

        /* Restart essential task here */

        /* Turn on On/Sleep Led when we are awake */
        suli_pin_write(&SleepLed, HAL_PIN_HIGH);

        if(SLEEP_MODE_WAKE_BY_DIO_TIMER == g_sDevice.config.sleepMode)
        {
        	/* Clean DIO interrupt flag and activate WakeUpTask */
			if(WAKE_BTN & u32AHI_DioWakeStatus())
			{
				DBG_vPrintf(TRACE_START, "Wake up by DIO.\n");
				OS_eActivateTask(WakeUpTask);
			}
        }
    }
}


/****************************************************************************
 *
 * NAME: vAppRegisterPWRMCallbacks
 *
 * DESCRIPTION:
 * Register call back
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
void vAppRegisterPWRMCallbacks(void)
{
    PWRM_vRegisterPreSleepCallback(PreSleep);
    PWRM_vRegisterWakeupCallback(Wakeup);
}


/* Wakeup timer callback */
//void PWRM_vWakeInterruptCallback()
//{
//
//}

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{
    //vInitStackMeasure();

    /*
     * Check that the clock source is the external 32MHz, needed for
     * accurate UART timings
    */
    while (bAHI_GetClkSource() == TRUE);
    // Now we are running on the XTAL, optimize the flash memory wait states.
    vAHI_OptimiseWaitStates();

    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);
    DBG_vPrintf(TRACE_START, "\r\n\r\n");
    DBG_vPrintf(TRACE_START, "=================================\r\n");
    DBG_vPrintf(TRACE_START, "            Mesh Bee \r\n");
    DBG_vPrintf(TRACE_START, "  Zigbee module from seeedstudio \r\n");
    DBG_vPrintf(TRACE_START, "     Firmware Version: 0x%04x \r\n", FW_VERSION);
    DBG_vPrintf(TRACE_START, "=================================\r\n");

    DBG_vPrintf(TRACE_START, "Stack overflow mark: 0x%08x\r\n", &stack_low_water_mark);

    vAHI_SetStackOverflow(TRACE_START, (uint32)&stack_low_water_mark);

    DBG_vPrintf(TRACE_START, "Stack Size: %d\r\n",    (uint32)&stack_size);

    DBG_vPrintf(TRACE_START, "Free RAM: %d\r\n",    (uint32)&free_ram_len);

    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRACE_START, "Last reset is caused by Watchdog\r\n");

        /* invoking here means software has some lock-up situations, stop it */
        vAHI_WatchdogStop();
        //while (1);  //let watchdog reset the CPU
    }

    u32AppApiInit(NULL, NULL, NULL, NULL, NULL, NULL);

    vAHI_HighPowerModuleEnable(TRUE, TRUE);

#ifdef ETSI
    vAHI_ETSIHighPowerModuleEnable(TRUE);                                   // Limit the module to +8dB for ETSI compliance
#endif

    /* Start the RTOS */
    OS_vStart(vInitialiseApp, vUnclaimedInterrupt, vOSError);

    /* idle task(Lowest priority task) commences on exit from OS start call */
    while (TRUE)
    {
        /*
         * Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed within the watchdog period
         * otherwise the system will be reset.
        */
        vAHI_WatchdogRestart();

        /*
         * suspends CPU operation when the system is idle or puts the device to
         * sleep if there are no activities in progress
        */
#ifdef TARGET_END
        PWRM_vManagePower();
#endif
    }
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vSetUpWakeUpConditions
 *
 * DESCRIPTION:
 *
 * Set up the wake up inputs while going to sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetUpWakeUpConditions(void)
{
   /*
    * Set the DIO with the right stages for wake up
    * */

    vAHI_DioSetDirection(WAKE_BTN,0);	/* Set as Wake up Button(DIO0) as Input */
    vAHI_DioWakeEdge(0,WAKE_BTN);   	/* Set the wake up DIO Edge - Falling Edge */
    vAHI_DioWakeEnable(WAKE_BTN,0); 	/* Set the Wake up DIO Power Button */
}
/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{
    /* initialise JenOS modules */

#ifdef TARGET_END
    /* xtal on, ram on */
	DBG_vPrintf(TRACE_START, "Initialising PWRM ... \r\n");
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);
#endif

    DBG_vPrintf(TRACE_START, "Initialising PDM ... \r\n");

    PDM_vInit(0, 63, 64, NULL, NULL, NULL, NULL);
    PDM_vRegisterSystemCallback(vPdmEventHandlerCallback);

    DBG_vPrintf(TRACE_START, "Initialising PDUM ... \r\n");
    PDUM_vInit();

    char *role = "COO";
#if defined(TARGET_ROU)
    role = "ROU";
#elif defined(TARGET_END)
    role = "END";
#endif

    DBG_vPrintf(TRACE_START, "Initialising %s node... \r\n", role);
    node_vInitialise();
}


/****************************************************************************
 *
 * NAME: vOSError
 *
 * DESCRIPTION:
 * Catches any unexpected OS errors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject)
{
    OS_thTask hTask;

    /* ignore queue underruns */
    if (OS_E_QUEUE_EMPTY == eStatus)
    {
        return;
    }

    DBG_vPrintf(TRACE_EXCEPTION, "OS Error %d, offending object handle = 0x%08x\r\n", eStatus, hObject);

    /* NB the task may have been pre-empted by an ISR which may be at fault */
    OS_eGetCurrentTask(&hTask);
    DBG_vPrintf(TRACE_EXCEPTION, "Currently active task handle = 0x%08x\r\n", hTask);

#ifdef OS_STRICT_CHECKS
    DBG_vPrintf(TRACE_EXCEPTION, "Currently active ISR fn address = 0x%08x\r\n", OS_prGetActiveISR());
#endif
}


/****************************************************************************
 *
 * NAME: vUnclaimedInterrupt
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vUnclaimedInterrupt(void)
{
    register uint32 u32PICSR, u32PICMR;

    asm volatile("l.mfspr %0,r0,0x4800" :"=r"(u32PICMR) :);
    asm volatile("l.mfspr %0,r0,0x4802" :"=r"(u32PICSR) :);

    DBG_vPrintf(TRACE_EXCEPTION, "Unclaimed interrupt : %x : %x\r\n", u32PICSR, u32PICMR);

    while (1);
}


/****************************************************************************
 *
 * NAME: APP_isrBusErrorException
 *
 * DESCRIPTION:
 * Catches any bus error exceptions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrBusErrorException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Bus error\r\n");
    DBG_vDumpStack();
    while (1);
}


/****************************************************************************
 *
 * NAME: APP_isrAlignmentException
 *
 * DESCRIPTION:
 * Catches any address alignment exceptions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrAlignmentException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Align error\r\n");
    DBG_vDumpStack();
    while (1);
}


/****************************************************************************
 *
 * NAME: APP_isrIllegalInstructionException
 *
 * DESCRIPTION:
 * Catches any illegal instruction exceptions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrIllegalInstructionException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Illegal error\r\n");
    DBG_vDumpStack();
    while (1);
}


/****************************************************************************
 *
 * NAME: APP_isrStackOverflowException
 *
 * DESCRIPTION:
 * Catches any stack overflow exceptions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrStackOverflowException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "StackOverflow error\r\n");
    DBG_vDumpStack();
    while (1);
}


/****************************************************************************
 *
 * NAME: APP_isrUnimplementedModuleException
 *
 * DESCRIPTION:
 * Catches any unimplemented module exceptions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrUnimplementedModuleException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Unimplemented error\r\n");
    DBG_vDumpStack();
    while (1);
}


#ifdef PDM_EEPROM
/****************************************************************************
 *
 * NAME: vPdmEventHandlerCallback
 *
 * DESCRIPTION:
 * Handles PDM callback, information the application of PDM conditions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode)
{

    switch (eSystemEventCode)
    {
        /*
         * The next three events will require the application to take some action
         */
    case E_PDM_SYSTEM_EVENT_WEAR_COUNT_TRIGGER_VALUE_REACHED:
        DBG_vPrintf(TRACE_START, "PDM: Segment %d reached trigger wear level\r\n", u32EventNumber);
        break;
    case E_PDM_SYSTEM_EVENT_DESCRIPTOR_SAVE_FAILED:
        DBG_vPrintf(TRACE_START, "PDM: Record Id %d failed to save\r\n", u32EventNumber);
        DBG_vPrintf(TRACE_START, "PDM: Capacity %d\r\n", u8PDM_CalculateFileSystemCapacity());
        DBG_vPrintf(TRACE_START, "PDM: Occupancy %d\r\n", u8PDM_GetFileSystemOccupancy());
        break;
    case E_PDM_SYSTEM_EVENT_PDM_NOT_ENOUGH_SPACE:
        DBG_vPrintf(TRACE_START, "PDM: Record %d not enough space\r\n", u32EventNumber);
        DBG_vPrintf(TRACE_START, "PDM: Capacity %d\r\n", u8PDM_CalculateFileSystemCapacity());
        DBG_vPrintf(TRACE_START, "PDM: Occupancy %d\r\n", u8PDM_GetFileSystemOccupancy());
        break;

        /*
         *  The following events are really for information only
         */
    case E_PDM_SYSTEM_EVENT_EEPROM_SEGMENT_HEADER_REPAIRED:
        DBG_vPrintf(TRACE_START, "PDM: Segment %d header repaired\r\n", u32EventNumber);
        break;
    case E_PDM_SYSTEM_EVENT_SYSTEM_INTERNAL_BUFFER_WEAR_COUNT_SWAP:
        DBG_vPrintf(TRACE_START, "PDM: Segment %d buffer wear count swap\r\n", u32EventNumber);
        break;
    case E_PDM_SYSTEM_EVENT_SYSTEM_DUPLICATE_FILE_SEGMENT_DETECTED:
        DBG_vPrintf(TRACE_START, "PDM: Segement %d duplicate selected\r\n", u32EventNumber);
        break;
    default:
        DBG_vPrintf(TRACE_START, "PDM: Unexpected call back Code %d Number %d\r\n", eSystemEventCode, u32EventNumber);
        break;
    }
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
