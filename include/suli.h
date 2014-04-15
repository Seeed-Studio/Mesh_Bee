/*
 * suli.h
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
#ifndef __SEEED_UNIFIED_LIBRARY_INTERFACE_H__
#define __SEEED_UNIFIED_LIBRARY_INTERFACE_H__

#include "common.h"


/**
 * GPIO TYPE, it means the data type you gpio name, 
 * such as, for Arduino, we use pinMode(pin, INPUT), and pin is int. 
 * but for mbed, it's gpio_t
 * For porting, you should modify here
 */
typedef     int     IO_T;                        // IO type
typedef     int     PIN_T;                      // pin name
typedef     int     DIR_T;                 // pin direction

typedef     unsigned char   ANALOG_T;                        // pin analog


/** 
 * PIN MODE
 * INPUT or OUTPUT
 */
#define HAL_PIN_INPUT   0x00                   // INPUT and OUTPUT was declared in Arduino IDE
#define HAL_PIN_OUTPUT  0x01


/**
 * PIN STATE
 * HIGH or LOW
 */
#define HAL_PIN_HIGH    0x01                        // HIGH and LOW was declered in Arduino IDE
#define HAL_PIN_LOW     0x00

/**
 * PIN DEFINATIONS 
 * For DIO pins, you can use 0~20 or D0~D20 
 * For DO pins, you can just specify DO0/DO1 
 */
enum
{
    D0 = 0, D1=1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18, D19, D20, DO0=33, DO1=34
}; 

enum
{
    A3=0, A4=1, A2=50, A1, TEMP, VOL 
}; 


/**
 * DATA TYPE
 * ALL our suly-compatible library will use those data type 
 * (defined in jendefs.h) 
 */
//typedef signed char    int8;
//typedef unsigned char  uint8;
//typedef signed short   int16;
//typedef unsigned short uint16;
//typedef signed long    int32;
//typedef unsigned long  uint32;

/**
 * SULI INIT 
 * Suli should be inited before any suli_* function call 
 * Arduino & mbed platform linked some hardware initial code 
 * background, so no need to call suli_init. 
 */
void suli_init(void);


/**
 * Digital IO Operation
 * when use an IO, this IO should be initialized first.
 */
void suli_pin_init(IO_T *pio, PIN_T pin);      // pin initialize
void suli_pin_dir(IO_T *pio, DIR_T dir);       // set pin direction
void suli_pin_write(IO_T *pio, int16 state);   // write pin
int16 suli_pin_read(IO_T *pio);                // read pin


/**
 * Reads a pulse (either HIGH or LOW) on a pin. For example, if value is HIGH, 
 * suli_pulse_in() waits for the pin to go HIGH, starts timing, 
 * then waits for the pin to go LOW and stops timing. Returns the length of the pulse in microseconds. 
 * Gives up and returns 0 if no pulse starts within a specified time out.
 * para -
 * - pin: pins which you want to read the pulse.
 * - state: type of pulse to read: either HIGH or LOW. (int)
 * - timeout (optional): the number of microseconds to wait for the pulse to start; default is one second (unsigned long)
 */
uint16 suli_pulse_in(IO_T *pio, uint8 state, uint32 timeout);


/*
 * Analog IO Operation
 * As usually, 10bit ADC is enough, to increase the compatibility, will use only 10bit.
 * if if your ADC is 12bit, you need to >>2, or your ADC is 8Bit, you need to <<2
 */
void suli_analog_init(ANALOG_T * aio, PIN_T pin);
int16 suli_analog_read(ANALOG_T * aio);



/*
 * delay
 */
void suli_delay_us(uint32 us);                 // delay us
void suli_delay_ms(uint32 ms);                 // delay ms


/*
 * Returns the number of milliseconds since your board began running the current program. 
 * This number will overflow (go back to zero), after approximately 50 days.
*/
uint32 suli_millis(void);


/*
 * Returns the number of microseconds since your board began running the current program. 
 * This number will overflow (go back to zero), after approximately 70 minutes.
 * Note: there are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second.
 */
uint32 suli_micros(void);


// I2C

/*
 * I2C interface initialize. 
 */
void suli_i2c_init(void * i2c_device);


/*
 * write a buff to I2C
 */
uint8 suli_i2c_write(void * i2c_device, uint8 dev_addr, uint8 *data, uint8 len);

/*
 * read data from I2C
 */
uint8 suli_i2c_read(void * i2c_device, uint8 dev_addr, uint8 *buff, uint8 len);


// UART

/*
 * uart init
 */
void suli_uart_init(void * uart_device, int16 uart_num, uint32 baud);


/*
 * send a buff to uart
 */
void suli_uart_send(void * uart_device, int16 uart_num, uint8 *data, uint16 len);


/*
 * send a byte to uart
 */
void suli_uart_send_byte(void * uart_device, int16 uart_num, uint8 data);


/*
 * read a byte from uart
 */
uint8 suli_uart_read_byte(void * uart_device, int16 uart_num);


/*
 * if uart get data, return 1-readable, 0-unreadable
 */
uint16 suli_uart_readable(void * uart_device, int16 uart_num);

#endif
