/*
 * firmware_hal.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Oliver Wang
 * Create Time: 2014/3
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
#include "firmware_hal.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_HAL
#define TRACE_HAL TRUE
#endif

#define TEMP_XTAL_HALF_PULL             95  /*  95C */
#define TEMP_XTAL_HALF_PUSH             93  /*  93C */
#define TEMP_XTAL_FULL_PULL            110  /* 110C */
#define TEMP_XTAL_FULL_PUSH            108  /* 108C */

#define ADC_REG                        0x02001f04
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vHAL_AdcSampleInit
 *
 * DESCRIPTION:
 * init ADC
 *
 * PARAMETERS: u8Source       tsAdcParam*
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHAL_AdcSampleInit(tsAdcParam *param)
{
	/* Set up the analog peripherals,ready to handle the conversions */
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
					 E_AHI_AP_INT_DISABLE,
		    		 param->u8SampleSelect,
		    		 param->u8ClockDivRatio,
		    		 param->bRefSelect);

	/* Wait until the regulator becomes stable. */
	while(!bAHI_APRegulatorEnabled());

	DBG_vPrintf(TRACE_HAL, "Initializing ADC ...\r\n");
}
/****************************************************************************
 *
 * NAME: vHAL_AdcSampleRead
 *
 * DESCRIPTION:
 * ADC read
 *
 * PARAMETERS:  void
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
uint16 vHAL_AdcSampleRead(uint8 u8Source)
{
	uint16 iAdVal = 0;		//local value

	/* Enable ADC */
	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, u8Source);

	/* Start Sample */
	vAHI_AdcStartSample();

	/* Wait until ADC data is available */
	while(bAHI_AdcPoll());

	/* Here return register value */
	iAdVal = *(uint32 *)ADC_REG;

	return iAdVal;
}

/****************************************************************************
 *
 * NAME: i16HAL_GetChipTemp
 *
 * DESCRIPTION:
 * Helper Function to convert 10bit ADC reading to degrees C
 * Formula: DegC = Typical DegC - ((Reading12 - Typ12) * ScaleFactor)
 * Where C = 25 and temps sensor output 730mv at 25C (from datasheet)
 * As we use 2Vref and 10bit adc this gives (730/2400)*4096  [=Typ12 =1210]
 * Scale factor is half the 0.706 data-sheet resolution DegC/LSB (2Vref)
 *
 * PARAMETERS:  u16AdcValue
 *
 *
 * RETURNS:
 * Chip Temperature in DegC
 *
 ****************************************************************************/
PUBLIC int16 i16HAL_GetChipTemp(uint16 u16AdcValue)
{
	int16 i16Centigrade;

	i16Centigrade = (int16) ((int32) 25 - ((((int32) (u16AdcValue*4) - (int32) 1210) * (int32) 353) / (int32) 1000));

	return (i16Centigrade);
}


/****************************************************************************
 *
 * NAME: vHAL_PullXtal
 *
 * DESCRIPTION:
 * Oscillator pulling State machine
 *
 * PARAMETERS:  void
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHAL_PullXtal(int32 i32Temperature)
{
	static teXtalPullingStates eXtalPullingState = E_STATE_XTAL_UNPULLED;

    DBG_vPrintf(TRACE_HAL, "\nAPP: T =%d C",i32Temperature);

	switch (eXtalPullingState)
	{
		case  E_STATE_XTAL_UNPULLED :
			if (i32Temperature >= TEMP_XTAL_HALF_PULL)
			{
				DBG_vPrintf(TRACE_HAL, "\nAPP: Xtal 1/2 pulled");
				eXtalPullingState = E_STATE_XTAL_SEMIPULLED;
				vAHI_ClockXtalPull(eXtalPullingState);
			}
			break;

		case  E_STATE_XTAL_SEMIPULLED :
			if (i32Temperature >= TEMP_XTAL_FULL_PULL)
			{
				DBG_vPrintf(TRACE_HAL, "\nAPP: Xtal full pulled");
				eXtalPullingState = E_STATE_XTAL_PULLED;
				vAHI_ClockXtalPull(eXtalPullingState);
			}
			else if (i32Temperature < TEMP_XTAL_HALF_PUSH)
			{
				DBG_vPrintf(TRACE_HAL, "\nAPP: Xtal not pulled");
				eXtalPullingState = E_STATE_XTAL_UNPULLED;
				vAHI_ClockXtalPull(eXtalPullingState);
			}
			break;

		case  E_STATE_XTAL_PULLED :
			if (i32Temperature < TEMP_XTAL_FULL_PUSH)
			{
				DBG_vPrintf(TRACE_HAL, "\nAPP: Xtal 1/2 pulled");
				eXtalPullingState = E_STATE_XTAL_SEMIPULLED;
				vAHI_ClockXtalPull(eXtalPullingState);
			}
			break;

		default :
		break;
	}
}


/****************************************************************************
 *
 * NAME: vHAL_UartRead
 *
 * DESCRIPTION:
 * Read data from AUPS ringbuffer
 *
 * PARAMETERS:  len
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vHAL_UartRead(void *data, int len)
{
    uint32 dataCnt = 0;

    OS_eEnterCriticalSection(mutexRxRb);
    dataCnt = ringbuffer_data_size(&rb_uart_aups);
    if(dataCnt >= len)
    {
        ringbuffer_read(&rb_uart_aups, data, len);
    }
    OS_eExitCriticalSection(mutexRxRb);
}
