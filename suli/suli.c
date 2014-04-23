/*
 * suli.c
 * Seeed Unified Library Interface for Mesh Bee
 * 
 * About Suli: https://github.com/Seeed-Studio/Suli
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
void modp_dtoa2(double value, char *str, int prec);
static void strreverse(char *begin, char *end);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
volatile uint32 timer0_overflow_count = 0;
volatile uint32 timer0_millis = 0; 

static const double pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000,
                                10000000, 100000000, 1000000000 }; 

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
    return ((u32AHI_DioReadInput() >> (*pio)) & 0x1);
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
uint32 suli_pulse_in(IO_T *pio, uint8 state, uint32 timeout)
{
    #define LOOP_CYCLES   38
    uint32 width = 0;
    uint32 numloops = 0;
    uint32 maxloops = timeout * 32 / (LOOP_CYCLES-5); 

    while(suli_pin_read(pio) == state)
    {
        if (numloops++ == maxloops)
			return 0;
    }

    while(suli_pin_read(pio) != state)
    {
        if (numloops++ == maxloops)
			return 0;
    }

    while(((u32AHI_DioReadInput() >> (*pio)) & 0x1) == state) 
    {
        if (numloops++ == maxloops)
			return 0;
		width++;
    }
    return (width * LOOP_CYCLES + 32) / 32; 
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
    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, *aio);  //2*vref = 1.2*2 = 2.4V
    vAHI_AdcStartSample();
    // Wait until ADC data is available
    while(bAHI_AdcPoll());
    
    uint16 val = u16AHI_AdcRead();
    
    return val;
    
    //convert the output to 5V - 1024
    //return (int16)(val * 6.6f / 5.0f);   
}


/*
 * delay us
 */
void suli_delay_us(uint32 us)
{
    uint32 mark_time = suli_micros();
    while(suli_micros() - mark_time < us); 
}


/*
 * delay ms
 */
void suli_delay_ms(uint32 ms)
{
    uint32 mark_time = suli_millis();
    while(suli_millis() - mark_time < ms);
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
void suli_uart_send_byte(void *uart_device, int16 uart_num, uint8 data)
{
    uart_tx_data(&data, 1);
}

/*
 * write a float
 * num - number to write
 * decimal - x decimal point
 */
void suli_uart_write_float(void *uart_device, int16 uart_num, float data, uint8 prec)
{
    char buff[32];
    modp_dtoa2((double)data, buff, prec);
    uart_tx_data(buff, strlen(buff)); 
}

/*
 * write an integer
 * num - number to write
 */
void suli_uart_write_int(void *uart_device, int16 uart_num, int32 num)
{
    suli_uart_printf(uart_device, uart_num, "%ld", num);
}


/*
 * send formatted string to uart 
 * max length after formateed: 80 
 */
void suli_uart_printf(void *uart_device, int16 uart_num, const char *fmt, ...)
{
    char buff[82];
    va_list args;
    int n;

    va_start(args, fmt);
    n = vsnprintf(buff, 80, fmt, args);
    va_end(args);

    uart_tx_data(buff, n);
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

static void strreverse(char *begin, char *end)
{
    char aux;
    while(end > begin) aux = *end,*end-- = *begin,*begin++ = aux;
}

void modp_dtoa2(double value, char *str, int prec)
{
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if(!(value == value))
    {
        str[0] = 'n'; str[1] = 'a'; str[2] = 'n'; str[3] = '\0';
        return;
    }

    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);

    int count;
    double diff = 0.0;
    char *wstr = str;

    if(prec < 0)
    {
        prec = 0;
    } else if(prec > 9)
    {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }


    /* we'll work in positive values and deal with the
       negative sign issue later */
    int neg = 0;
    if(value < 0)
    {
        neg = 1;
        value = -value;
    }


    int whole = (int)value;
    double tmp = (value - whole) * pow10[prec];
    uint32 frac = (uint32)(tmp);
    diff = tmp - frac;

    if(diff > 0.5)
    {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if(frac >= pow10[prec])
        {
            frac = 0;
            ++whole;
        }
    } else if(diff == 0.5 && ((frac == 0) || (frac & 1)))
    {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
      normal printf behavior is to print EVERY whole number digit
      which can be 100s of characters overflowing your buffers == bad
    */
    if(value > thres_max)
    {
        sprintf(str, "%e", neg ? -value : value);
        return;
    }

    if(prec == 0)
    {
        diff = value - whole;
        if(diff > 0.5)
        {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        } else if(diff == 0.5 && (whole & 1))
        {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }

        //vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
    } else if(frac)
    {
        count = prec;
        // now do fractional part, as an unsigned number
        // we know it is not 0 but we can have leading zeros, these
        // should be removed
        while(!(frac % 10))
        {
            --count;
            frac /= 10;
        }
        //^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

        // now do fractional part, as an unsigned number
        do
        {
            --count;
            *wstr++ = (char)(48 + (frac % 10));
        } while(frac /= 10);
        // add extra 0s
        while(count-- > 0) *wstr++ = '0';
        // add decimal
        *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (whole % 10)); while(whole /= 10);
    if(neg)
    {
        *wstr++ = '-';
    }
    *wstr = '\0';
    strreverse(str, wstr - 1);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
