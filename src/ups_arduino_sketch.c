/*
 * ups_arduino_sketch.c
 * User programming space, arduino sketch file
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

#include "common.h"
#include "firmware_api_pack.h"
#include "firmware_at_api.h"
#include "firmware_aups.h"
#include "firmware_sleep.h"
#include "suli.h"


/****************************************************************************/
/***       example 1 - for test suli - with xbee carrier board            ***/
/****************************************************************************/
/*
#include "ACC_Adxl345_Suli.h"
IO_T led_io;
IO_T input0_io;
IO_T input1_io;
IO_T xbee_carrier_power_ctl_io;
ANALOG_T input0_aio;
ANALOG_T input1_aio;
uint8 led_st  = HAL_PIN_LOW;

void arduino_setup(void)
{
    suli_pin_init(&led_io, D9);
    suli_pin_dir(&led_io, HAL_PIN_OUTPUT);

    suli_pin_init(&input0_io, D0);
    suli_pin_dir(&input0_io, HAL_PIN_INPUT);

    suli_pin_init(&input1_io, D1);
    suli_pin_dir(&input1_io, HAL_PIN_INPUT);

    //suli_analog_init(&input1_aio, A4);
    //suli_analog_init(&input0_aio, A3);

    suli_pin_init(&xbee_carrier_power_ctl_io, D12);
    suli_pin_dir(&xbee_carrier_power_ctl_io, HAL_PIN_OUTPUT);
    suli_pin_write(&xbee_carrier_power_ctl_io, 0);  //gate the mosfet to power the vcc_out2 of xbee_carrier_board

    suli_i2c_init(NULL);

    acc_adxl345_init(NULL);

    setLoopIntervalMs(10);   //default: 1000ms per loop

    suli_uart_printf(NULL, NULL, "Setup done.\r\n");
}

void arduino_loop(void)
{
    suli_pin_write(&led_io, led_st);
    led_st = ~led_st;
    suli_uart_printf(NULL, NULL, "millis: %lu \r\n", suli_millis());
    suli_uart_printf(NULL, NULL, "micros: %lu \r\n", suli_micros());
    //suli_uart_printf(NULL, NULL, "input pin 0: %d \r\n", suli_pin_read(&input0_io));
    //suli_uart_printf(NULL, NULL, "input pin 1: %d \r\n", suli_pin_read(&input1_io));
    //suli_uart_printf(NULL, NULL, "input pin 1: %d \r\n", suli_analog_read(&input1_aio));
    //suli_uart_printf(NULL, NULL, "input pin 0: %d \r\n", suli_analog_read(&input0_aio));

    //suli_uart_printf(NULL, NULL, "input pin 1 pulse: %lu us \r\n", suli_pulse_in(&input1_io, HAL_PIN_HIGH, 1000000));

    //suli_delay_ms(suli_analog_read(&input1_aio));
    //suli_delay_us(suli_analog_read(&input1_aio));

    float ax,ay,az;
    acc_adxl345_read_acc(&ax, &ay, &az);

    suli_uart_printf(NULL, NULL, "ax: ");
    suli_uart_write_float(NULL, NULL, ax, 2);
    suli_uart_printf(NULL, NULL, ", ay: ");
    suli_uart_write_float(NULL, NULL, ay, 2);
    suli_uart_printf(NULL, NULL, ", az: ");
    suli_uart_write_float(NULL, NULL, az, 2);
    suli_uart_printf(NULL, NULL, "\r\n");
}
*/

//IO_T led_io;
//int16 state = HAL_PIN_HIGH;
ANALOG_T temp_pin;

void arduino_setup(void)
{
    //setLoopIntervalMs(1000);   //default: 500ms per loop
    suli_analog_init(&temp_pin, TEMP);
}

void arduino_loop(void)
{
#ifdef TARGET_COO
    vDelayMsec(100);
    suli_uart_printf(NULL, NULL, "random:%d\r\n", random());
#elif TARGET_ROU
    uint8 tmp[sizeof(tsApiSpec)]={0};
    tsApiSpec apiSpec;

    int16 temper = suli_analog_read(temp_pin);
    sprintf(tmp, "R-HeartBeat:%ld\r\n", temper);
    PCK_vApiSpecDataFrame(&apiSpec, 0xec, 0x00, tmp, strlen(tmp));

    /* Air to Coordinator */
    uint16 size = i32CopyApiSpec(&apiSpec, tmp);
    if(API_bSendToAirPort(UNICAST, 0x0000, tmp, size))
    {
        suli_uart_printf(NULL, NULL, "<HeartBeat%d>\r\n", random());
    }
#else
    /* Finish user job */
    static jobCnt = 0;
    uint8 tmp[sizeof(tsApiSpec)]={0};
    tsApiSpec apiSpec;

    int16 temper = suli_analog_read(temp_pin);
    sprintf(tmp, "E-HeartBeat:%ld\r\n", temper);
    PCK_vApiSpecDataFrame(&apiSpec, 0xec, 0x00, tmp, strlen(tmp));

    /* Air to Coordinator */
    uint16 size = i32CopyApiSpec(&apiSpec, tmp);
    if(API_bSendToAirPort(UNICAST, 0x0000, tmp, size))
    {
        suli_uart_printf(NULL, NULL, "<HeartBeat%d>\r\n", jobCnt);
        jobCnt++;
    }
    if(10 == jobCnt)
    {
        jobCnt = 0;
        Sleep(3000);
    }

#endif
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
