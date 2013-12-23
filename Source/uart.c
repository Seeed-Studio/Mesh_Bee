/*    
 * uart.c
 * Firmware for SeeedStudio RFBeeV2(Zigbee) module 
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

#include "common.h"
#include "uart.h"


#ifndef TRACE_UART
#define TRACE_UART TRUE
#endif


unsigned char txfifo[TXFIFOLEN];
unsigned char rxfifo[RXFIFOLEN];
PRIVATE  volatile bool txbusy = FALSE;

/****************************************************************************
 *
 * NAME: uart_initialize
 *
 * DESCRIPTION:
 * init uart1
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void uart_initialize()
{
    if (!bAHI_UartEnable(UART_COMM, txfifo, TXFIFOLEN, rxfifo, RXFIFOLEN))
    {
        DBG_vPrintf(TRACE_UART, "UART1 enable fail \r\n");
        return;
    }
    AT_setBaudRateUart1(&g_sDevice.config.baudRateUart1);
    vAHI_UartSetControl(UART_COMM, E_AHI_UART_EVEN_PARITY, E_AHI_UART_PARITY_DISABLE, E_AHI_UART_WORD_LEN_8, E_AHI_UART_1_STOP_BIT, FALSE);
    vAHI_UartSetInterrupt(UART_COMM, FALSE, FALSE, TRUE, TRUE, E_AHI_UART_FIFO_LEVEL_1);
    DBG_vPrintf(TRACE_UART, "UART1 enabled \r\n");
}

/****************************************************************************
 *
 * NAME: AT_setBaudRateUart1
 *
 * DESCRIPTION:
 * set buad rate for uart1
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   poiter to a uint16 that containing the value of baudrate index
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
int AT_setBaudRateUart1(uint16 *regAddr)
{
    //#define E_AHI_UART_RATE_4800       0
    //#define E_AHI_UART_RATE_9600       1
    //#define E_AHI_UART_RATE_19200      2
    //#define E_AHI_UART_RATE_38400      3
    //#define E_AHI_UART_RATE_76800      4
    //#define E_AHI_UART_RATE_115200     5
    if (*regAddr > 5) *regAddr = 5;
    vAHI_UartSetBaudRate(UART_COMM, *regAddr);
}

/****************************************************************************
 *
 * NAME: APP_isrUART1
 *
 * DESCRIPTION:
 * ISR for uart1
 * 
 ****************************************************************************/
OS_ISR(APP_isrUART1)
{
    uint8 intrpt;
    uint32 free_cnt;
    uint32 avlb_cnt;
    uint32 cnt;

    intrpt = (u8AHI_UartReadInterruptStatus(UART_COMM) >> 1) & 0x7;

    DBG_vPrintf(TRACE_UART, "\r\nuart interrupt: %d \r\n", intrpt);
    if (intrpt == E_AHI_UART_INT_RXDATA)
    {
        avlb_cnt = u16AHI_UartReadRxFifoLevel(UART_COMM);

        if (avlb_cnt > 0)
        {
            uint8 tmp[RXFIFOLEN];

            u16AHI_UartBlockReadData(UART_COMM, tmp, avlb_cnt); //anyhow we read to empty to clear interrupt flag
                                                                //if not do so, ISR will occur again and again
            OS_eEnterCriticalSection(mutexRxRb);
            free_cnt = ringbuffer_free_space(&rb_rx_uart);
            OS_eExitCriticalSection(mutexRxRb);
            cnt = MIN(free_cnt, avlb_cnt);
            DBG_vPrintf(TRACE_UART, "avlb_cnt: %u, free_cnt: %u \r\n", avlb_cnt, free_cnt);
            if (cnt > 0)
            {
                OS_eEnterCriticalSection(mutexRxRb);
                ringbuffer_push(&rb_rx_uart, tmp, cnt);
                OS_eExitCriticalSection(mutexRxRb);

                OS_eActivateTask(APP_taskHandleUartRx);
            }
        }
    } else if (intrpt == E_AHI_UART_INT_TX) //tx empty
    {
        uart_trigger_tx();
    }
}

/****************************************************************************
 *
 * NAME: uart_get_tx_status_busy
 *
 * DESCRIPTION:
 * get uart1 tx status
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * bool: TRUE - busy
 * 
 ****************************************************************************/
bool uart_get_tx_status_busy()
{
    return txbusy;
}

/****************************************************************************
 *
 * NAME: uart_trigger_tx
 *
 * DESCRIPTION:
 * trigger the first tx action, the next actions will be trigger at ISR
 *
 * PARAMETERS: Name         RW  Usage
 *             
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void uart_trigger_tx()
{
    OS_eEnterCriticalSection(mutexTxRb);
    uint8 tmp[TXFIFOLEN];
    uint32 cnt = ringbuffer_data_size(&rb_tx_uart);

    cnt = MIN(TXFIFOLEN, cnt);

    if (cnt > 0)
    {
        ringbuffer_pop(&rb_tx_uart, tmp, cnt);
        u16AHI_UartBlockWriteData(UART_COMM, tmp, cnt);
        txbusy = TRUE;
    } else
    {
        txbusy = FALSE;
    }
    OS_eExitCriticalSection(mutexTxRb);
}

/****************************************************************************
 * NAME: uart_tx_data
 *
 * DESCRIPTION:
 * tx some amount of data
 *
 * PARAMETERS: Name         RW  Usage
 *             data         R   pointer to data buffer
 *             len          R   tx length
 *
 * RETURNS:
 * void
 ****************************************************************************/
void uart_tx_data(void *data, int len)
{
    uint32 free_cnt = 0;
    while (1)
    {
        OS_eEnterCriticalSection(mutexTxRb);
        free_cnt = ringbuffer_free_space(&rb_tx_uart);
        OS_eExitCriticalSection(mutexTxRb);
        if (free_cnt >= len)
            break;
    }
    OS_eEnterCriticalSection(mutexTxRb);
    ringbuffer_push(&rb_tx_uart, data, len);
    OS_eExitCriticalSection(mutexTxRb);

    if (!uart_get_tx_status_busy())
        uart_trigger_tx();
}

/****************************************************************************
 * NAME: uart_printf
 *
 * DESCRIPTION:
 * formatting print string to uart1
 *
 * PARAMETERS: Name         RW  Usage
 *             fmt          R   formatting string
 *             ...          R   var list
 *
 * RETURNS:
 * void
 ****************************************************************************/
int uart_printf(const char *fmt, ...)
{
    char buff[82];
	va_list args;
    int n;

	va_start(args, fmt);
	n = vsnprintf(buff, 80, fmt, args);
	va_end(args);
    
    uart_tx_data(buff, n);
    
    return n;
}



