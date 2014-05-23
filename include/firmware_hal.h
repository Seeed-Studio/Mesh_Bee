/*
 * firmware_hal.h
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

#ifndef FIRMWARE_HAL_H_
#define FIRMWARE_HAL_H_

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/* Pull state machine */
typedef enum
{
	E_STATE_XTAL_UNPULLED = 0,
	E_STATE_XTAL_SEMIPULLED = 1,
	E_STATE_XTAL_PULLED = 3
} teXtalPullingStates;


/* ADC configure parameter */
typedef struct
{
  bool_t bIntEnable;		// Enable/disable interrupt when ADC conversion completes
  uint8 u8SampleSelect;		// Sampling interval in terms of divided clock periods
  uint8 u8ClockDivRatio;	// Clock divisor (frequencies based on16MHz peripheral clock)
  bool_t bRefSelect;		// Source of reference voltage,Vref
  bool_t bContinuous;		// Conversion mode of ADC
  bool_t bInputRange;		// Input voltage range
}tsAdcParam;

PUBLIC void vHAL_AdcSampleInit(tsAdcParam *param);
PUBLIC uint16 vHAL_AdcSampleRead(uint8 u8Source);
PUBLIC void vHAL_PullXtal(int32 i32Temperature);
PUBLIC int16 i16HAL_GetChipTemp(uint16 u16AdcValue);
PUBLIC void vHAL_UartRead(void *data, int len);
PUBLIC uint16 random();
#endif /* FIRMWARE_HAL_H_ */
