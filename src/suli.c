/*
 * suli.c
 * Seeed Unified Library Interface for Mesh Bee
 *
 * 2013 Copyright (c) Seeed Technology Inc.  All right reserved.
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

#include "suli.h"
#include "firmware_uart.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_SULI
#define TRACE_SULI TRUE
#endif


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
volatile uint32 timer0_overflow_count = 0;
volatile uint32 timer0_millis = 0; 


/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/* 
 * initialize hardware 
 * e.g. timer for millis/micros/pulseIn 
 * For Arduino & mbed, no need to call this. 
 * Within Seeed firmware of Mesh Bee, we have called this at a proper place. 
 */
void suli_init(void)
{
    //init timer0 for millis/micros
    vAHI_TimerDIOControl(E_AHI_TIMER_0, FALSE);
    vAHI_TimerEnable(E_AHI_TIMER_0, 4, FALSE, TRUE, FALSE);  //1us/tick, period interrupt, no-output
    vAHI_TimerStartRepeat(E_AHI_TIMER_0, 30000, 60000);  //60ms/repeat
}

/*
 * IO initialize
 * *pio - IO
 * pin - pin name
 */
void suli_pin_init(IO_T *pio, PIN_T pin)
{
    *pio = pin;
    if(*pio >= DO0)
    {
        bAHI_DoEnableOutputs(true);  //Note: once you configured one of DO0/DO1 as digital output,
                                     //      both pin can not be used as SPI/PWM.
    }
}

/*
 * set IO direction
 * - pio: IO device pointer
 * - dir: INPUT or OUTPUT
 */
void suli_pin_dir(IO_T *pio, DIR_T dir)
{
    if(*pio >= DO0)
    {
        if(dir == HAL_PIN_INPUT) 
        {
            DBG_vPrintf(TRACE_SULI, "DO pins can not be configured as input.\r\n"); 
        }
    } else
    {
        uint32 u32Inputs = 0;
        uint32 u32Outputs = 0; 
    
        if(dir == HAL_PIN_INPUT)
        {
            u32Inputs = (1 << (*pio));
        } else
        {
            u32Outputs = (1 << (*pio));
        }
        vAHI_DioSetDirection(u32Inputs, u32Outputs);
    }
}


/*
 * write to IO
 * - pio: IO device pointer
 * - state: HIGH or LOW
 */
void suli_pin_write(IO_T *pio, int16 state)
{
    if(*pio >= DO0)
    {
        uint8 u8On = 0; 
        uint8 u8Off = 0; 
        
        if(state == HAL_PIN_LOW)
        {
            u8Off = (1 << (*pio - DO0)); 
        } else
        {
            u8On = (1 << (*pio - DO0)); 
        }
        vAHI_DoSetDataOut(u8On, u8Off);
    } else
    {
        uint32 u32On = 0;
        uint32 u32Off = 0;

        if(state == HAL_PIN_LOW)
        {
            u32Off = (1 << (*pio)); 
        } else
        {
            u32On = (1 << (*pio)); 
        }
        vAHI_DioSetOutput(u32On, u32Off); 
    }
}


/*
 * read IO state
 * - pio: IO device pointer
 * return HIGH or LOW
 */
int16 suli_pin_read(IO_T *pio)
{
    uint32 val = u32AHI_DioReadInput();
    return (val & (1 << (*pio)))> 0 ? HAL_PIN_HIGH: HAL_PIN_LOW;
}


/**
 * Reads a pulse (either HIGH or LOW) on a pin. For example, if value is HIGH, 
 * suli_pulse_in() waits for the pin to go HIGH, starts timing, 
 * then waits for the pin to go LOW and stops timing. Returns the length of the pulse in microseconds. 
 * Gives up and returns 0 if no pulse starts within a specified time out.
 * para -
 * pin: pins which you want to read the pulse.
 * state: type of pulse to read: either HIGH or LOW. (int)
 * timeout (optional): the number of microseconds to wait for the pulse to start; default is one second (unsigned long)
 */
 
 
uint32 suli_pulse_insuli_pulse_in(IO_T *pio, uint8 state, uint32 timeout)
{
    uint32 timer_cnt = suli_micros();

    while(suli_pin_read(pio) != state) 
    {
        if(timeout > 0 && (suli_micros() - timer_cnt) > timeout) return 0; 
    }

    timer_cnt = suli_micros(); 
    uint32 cur_time = timer_cnt; 
    while(suli_pin_read(pio) == state) 
    {
        cur_time = suli_micros();
        if(timeout > 0 && (cur_time - timer_cnt) > timeout) return 0; 
    }
    return cur_time - timer_cnt;
}


/*
 * Analog Init
 * - aio: gpio device pointer
 * - pin: pin name
 */
void suli_analog_init(ANALOG_T * aio, PIN_T pin)
{
    if(pin == A3 || pin == A4 || pin == A1 || pin == A2 || pin == TEMP || pin == VOL)
    {
        switch(pin)
        {
        case A3:
            *aio = E_AHI_ADC_SRC_ADC_3;
            break;
        case A4:
            *aio = E_AHI_ADC_SRC_ADC_4;
            break;
        case A1:
            *aio = E_AHI_ADC_SRC_ADC_1;
            break;
        case A2:
            *aio = E_AHI_ADC_SRC_ADC_2;
            break;
        case TEMP:
            *aio = E_AHI_ADC_SRC_TEMP;
            break;
        case VOL:
            *aio = E_AHI_ADC_SRC_VOLT;
            break;
        default:
            *aio = E_AHI_ADC_SRC_TEMP; 
        }
        vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
    					 E_AHI_AP_INT_DISABLE,
    		    		 E_AHI_AP_SAMPLE_8,
    		    		 E_AHI_AP_CLOCKDIV_500KHZ,
    		    		 E_AHI_AP_INTREF);

    	// Wait until the regulator becomes stable. 
    	while(!bAHI_APRegulatorEnabled());
    } else 
    {
        DBG_vPrintf(TRACE_SULI, "PIN %d can not be used as analog input.\r\n", pin); 
    }
}


/*
 * Analog Read
 * As usually, 10bit ADC is enough, to increase the compatibility, will use only 10bit.
 * if if your ADC is 12bit, you need to >>2, or your ADC is 8Bit, you need to <<2
 */
int16 suli_analog_read(ANALOG_T *aio)
{    
    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, *aio);  //2*vref = 6.6v
    vAHI_AdcStartSample();
    // Wait until ADC data is available
    while(bAHI_AdcPoll());
    
    uint16 val = u16AHI_AdcRead();
    
    //convert the output to 5V - 1024
    return (int16)((val >> 6) * 6.6f / 5.0f);   //16bit -> 10bit
}


/*
 * delay us
 */
#define APP_TIME_US(t)    (16UL*(t))
void suli_delay_us(uint32 us)
{
    //if (OS_eGetSWTimerStatus(Arduino_DelayTimer) != OS_E_SWTIMER_STOPPED)
    //{
    //    OS_eStopSWTimer(Arduino_DelayTimer); 
    //}
    OS_eStartSWTimer(Arduino_DelayTimer, APP_TIME_US(us), NULL); 
    while(OS_eGetSWTimerStatus(Arduino_DelayTimer) == OS_E_SWTIMER_RUNNING);
}


/*
 * delay ms
 */
void suli_delay_ms(uint32 ms)
{
    //if (OS_eGetSWTimerStatus(Arduino_DelayTimer) != OS_E_SWTIMER_STOPPED)
    //{
    //    OS_eStopSWTimer(Arduino_DelayTimer);
    //}
    OS_eStartSWTimer(Arduino_DelayTimer, APP_TIME_MS(ms), NULL);
    while(OS_eGetSWTimerStatus(Arduino_DelayTimer) == OS_E_SWTIMER_RUNNING); 
}


/*
 * Returns the number of milliseconds since your board began running the current program. 
 * This number will overflow (go back to zero), after approximately 50 days.
 */
uint32 suli_millis()
{
    return (u16AHI_TimerReadCount(E_AHI_TIMER_0)/1000 + 60 * timer0_overflow_count);
}


/*
 * Returns the number of microseconds since your board began running the current program. 
 * This number will overflow (go back to zero), after approximately 70 minutes.
 * Note: there are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second.
 */
uint32 suli_micros()
{
    return (u16AHI_TimerReadCount(E_AHI_TIMER_0) + 60000 * timer0_overflow_count); 
}


/*
 * I2C interface initialize. 
 */
void suli_i2c_init(void * i2c_device)
{
    vAHI_SiMasterConfigure(
        TRUE,  //bPulseSuppressionEnable,
        FALSE, //bInterruptEnable,
        31);   //uint8 u8PreScaler);  //16M/((scale+1)*5) = 100k
    vAHI_SiSetLocation(TRUE);  //D16,D17 as i2c
}


/*
 * write a buff to I2C
 * - i2c_device: i2c device pointer
 * - dev_addr: device address
 * - data: data buff
 * - len: data lenght
 */
uint8 suli_i2c_write(void * i2c_device, uint8 dev_addr, uint8 *data, uint8 len)
{
    vAHI_SiMasterWriteSlaveAddr(dev_addr, FALSE);
    // bSetSTA,  bSetSTO,  bSetRD,  bSetWR,  bSetAckCtrl,  bSetIACK);
    bAHI_SiMasterSetCmdReg(TRUE, FALSE, FALSE, TRUE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK); 
    while(bAHI_SiMasterPollTransferInProgress()); //Waitforanindicationofsuccess
    
    int i;
    uint8 *old = data;
    for(i = 0; i < len; i++)
    {
        vAHI_SiMasterWriteData8(*data++);
        if(i == (len - 1))  //should send stop
        {
            bAHI_SiMasterSetCmdReg(FALSE, TRUE, FALSE, TRUE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);
        } else
        {
            bAHI_SiMasterSetCmdReg(FALSE, FALSE, FALSE, TRUE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);
        }
        while(bAHI_SiMasterPollTransferInProgress()); //Waitforanindicationofsuccess
        if(bAHI_SiMasterCheckRxNack())
        {
            bAHI_SiMasterSetCmdReg(FALSE, TRUE, FALSE, FALSE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);
            break;
        }
    }
    return data - old;
}


/*
 * read a buff to I2C
 * - i2c_device: i2c device pointer
 * - dev_addr: device address
 * - data: data buff
 * - len: data lenght
 * return
 */
uint8 suli_i2c_read(void *i2c_device, uint8 dev_addr, uint8 *buff, uint8 len)
{
    vAHI_SiMasterWriteSlaveAddr(dev_addr, TRUE); 
    // bSetSTA,  bSetSTO,  bSetRD,  bSetWR,  bSetAckCtrl,  bSetIACK);
    bAHI_SiMasterSetCmdReg(TRUE, FALSE, FALSE, TRUE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK); 
    while(bAHI_SiMasterPollTransferInProgress()); //Waitforanindicationofsuccess

    int i;
    uint8 *old = buff;
    for(i = 0; i < len; i++)
    {
        if(i == (len - 1))  //should send stop, nack
        {
            bAHI_SiMasterSetCmdReg(FALSE, TRUE, TRUE, FALSE, E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK); 
        } else
        {
            bAHI_SiMasterSetCmdReg(FALSE, FALSE, TRUE, FALSE, E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK); 
        }
        while(bAHI_SiMasterPollTransferInProgress()); //Waitforanindicationofsuccess
        *buff++ = u8AHI_SiMasterReadData8();
    }
    return buff - old;
}


/*
 * UART Init
 * - uart_device: uart device pointer, IGNORED for Mesh Bee
 * - uart_num: for some MCU, there's more than one uart, this is the number of uart
 *   for Arduino-UNO(or others use 328, 32u4, 168), these is a hardware uart and software uart.
 *   for Mesh Bee (JN5168), IGNORED as only UART1 is for user 
 * - baud? baudrate
 */
void suli_uart_init(void * uart_device, int16 uart_num, uint32 baud)
{
    //#define E_AHI_UART_RATE_4800       0
    //#define E_AHI_UART_RATE_9600       1
    //#define E_AHI_UART_RATE_19200      2
    //#define E_AHI_UART_RATE_38400      3
    //#define E_AHI_UART_RATE_76800      //76800's not a well-used baudrate, we take 57600 instead.
    //#define E_AHI_UART_RATE_115200     5
    switch(baud)
    {
    case 4800:
        g_sDevice.config.baudRateUart1 = 0;
        break;
    case 9600:
        g_sDevice.config.baudRateUart1 = 1;
        break;
    case 19200:
        g_sDevice.config.baudRateUart1 = 2;
        break;
    case 38400:
        g_sDevice.config.baudRateUart1 = 3;
        break;
    case 57600:
        g_sDevice.config.baudRateUart1 = 4;
        break;
    case 115200:
        g_sDevice.config.baudRateUart1 = 5;
        break;
    default:
        g_sDevice.config.baudRateUart1 = 5;
        break;
    }
    PDM_vSaveRecord(&g_sDevicePDDesc); 
    AT_setBaudRateUart1(&g_sDevice.config.baudRateUart1); 
}


/*
 * Send a Buff to uart
 * - uart_device: uart device pointer
 * - uart_num: uart number
 * - *data: buff to sent
 * - len: data length
 */
void suli_uart_send(void * uart_device, int16 uart_num, uint8 *data, uint16 len)
{
    uart_tx_data(data, len);
}


/*
 * seed a byte to uart
 */
void suli_uart_send_byte(void * uart_device, int16 uart_num, uint8 data)
{
    uart_tx_data(&data, 1); 
}


/*
 * read a byte from uart
 */
uint8 suli_uart_read_byte(void * uart_device, int16 uart_num)
{
    uint32 dataCnt = 0; 
    char tmp;

    OS_eEnterCriticalSection(mutexRxRb);
    dataCnt = ringbuffer_data_size(&rb_rx_uart);
    if(dataCnt > 0)
    {
        ringbuffer_pop(&rb_rx_uart, &tmp, 1); 
    }
    OS_eExitCriticalSection(mutexRxRb); 

    if(dataCnt > 0) return tmp;
    else return 0;
}


/*
 * if uart get data, return 1-readable, 0-unreadable
 */
uint16 suli_uart_readable(void * uart_device, int16 uart_num)
{
    uint32 dataCnt = 0;
    OS_eEnterCriticalSection(mutexRxRb);
    dataCnt = ringbuffer_data_size(&rb_rx_uart); 
    OS_eExitCriticalSection(mutexRxRb); 
    return dataCnt;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/*
 * timer0's interrupt service routine
 */
OS_ISR(Suli_isrTimer0)
{
    u8AHI_TimerFired(E_AHI_TIMER_0);  //read to clear the interrupt flag
    timer0_overflow_count++;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
