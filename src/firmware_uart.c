/*
 * firmware_uart.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Jack Shao
 * Create Time: 2013/10
 * Change Log : Oliver Wang add CMI(communication interface layer) 2014/04/23
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
#include "firmware_uart.h"
#include "firmware_cmi.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_UART
#define TRACE_UART FALSE
#endif

extern void CMI_vPushData(void *data, int len);

/****************************************************************************/
/***        Exported Variables                                               ***/
/****************************************************************************/
unsigned char txfifo[TXFIFOLEN];
unsigned char rxfifo[RXFIFOLEN];
PRIVATE  volatile bool txbusy = FALSE;

/* for UART transfer data */
struct ringbuffer rb_tx_uart;
uint8 rb_tx_mempool[UART_TX_RB_LEN];


/****************************************************************************
 *
 * NAME: APP_isrUART1
 *
 * DESCRIPTION:
 * UART data server(UDS)
 * put received data into ringbuffer, or transfer data
 * Note: now we add communication interface layer,both input and output data
 *       were handled by CMI.
 *
 *       (1)Master Mode: External[data]-->suli-->AUPS_rx_ringbuffer-->AUPS
 *                    AUPS[data]-->AUPS_tx_ringbuffer-->suli-->External
 *       If user want to send a AT command,Steps:
 *       1.Pack one calling pack_lib;
 *       2.Push it into rb_rx_uart.
 *       3.A command execute thread will pop rb_rx_uart and execute callback
 *         function periodically.
 *       (2)Slave Mode:
 *                    External[apiSpec Frame]--rb_rx_uart
 *       A command execute thread will pop rb_rx_uart and execute
 *       callback function periodically.
 ****************************************************************************/
OS_ISR(APP_isrUART1)
{
    uint8 intrpt;
    uint32 avlb_cnt;

    intrpt = (u8AHI_UartReadInterruptStatus(UART_COMM) >> 1) & 0x7;

    DBG_vPrintf(TRACE_UART, "\r\nUART interrupt: %d \r\n", intrpt);
    if (intrpt == E_AHI_UART_INT_RXDATA)
    {
        avlb_cnt = u16AHI_UartReadRxFifoLevel(UART_COMM);

        if (avlb_cnt > 0)
        {
            uint8 tmp[RXFIFOLEN];
            /*
              anyhow we read to empty to clear interrupt flag
              if not do so, ISR will occur again and again
            */
            u16AHI_UartBlockReadData(UART_COMM, tmp, avlb_cnt);

        	/* if UART receive a event, sleep later */
#ifdef TARGET_END
        	if(E_MODE_API == g_sDevice.eMode || E_MODE_DATA == g_sDevice.eMode)
        	{
        	    vSleepSchedule();
        	}
#endif

            /* Push data into ringbuffer through CMI Distributor */
            CMI_vUrtRevDataDistributor(tmp, avlb_cnt);
        }
    }
    else if (intrpt == E_AHI_UART_INT_TX) //tx empty
    {
        uart_trigger_tx();
    }
}
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
    DBG_vPrintf(TRACE_UART, "UART1 enabled, baud rate: %d \r\n", g_sDevice.config.baudRateUart1);
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
    //#define E_AHI_UART_RATE_76800      //76800's not a well-used baudrate, we take 57600 instead.
    //#define E_AHI_UART_RATE_115200     5
    if (*regAddr > 5) *regAddr = 5;
    if (*regAddr == 4)
    {
      vAHI_UartSetBaudDivisor(UART_COMM, 23);  //57600bps
      vAHI_UartSetClocksPerBit(UART_COMM, 11);
    }
    else if (*regAddr == 5)
    {
      vAHI_UartSetBaudDivisor(UART_COMM, 10);
      vAHI_UartSetClocksPerBit(UART_COMM, 13);
    }
    else //others are acurate
    {
      vAHI_UartSetClocksPerBit(UART_COMM, 15);
      vAHI_UartSetBaudRate(UART_COMM, *regAddr);
    }
    return 0;
}
/****************************************************************************
 *
 * NAME: AT_printBaudRate
 *
 * DESCRIPTION:
 * print the buad rate for uart1
 *
 * PARAMETERS: Name         RW  Usage
 *             regAddr      R   poiter to a uint16 that containing the value of baudrate index
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
int AT_printBaudRate(uint16 *regAddr)
{
    uart_printf("%d\r\n", *regAddr);
    uart_printf("--------\r\n");
    uart_printf("Note: 0-4800, 1-9600, 2-19200, 3-38400, 4-57600, 5-115200\r\n");
    return 0;
}

/****************************************************************************
 *
 * NAME: ringbuf_vInitialize
 *
 * DESCRIPTION:
 * init ringbuffer for uart
 *
 * PARAMETERS: Name         RW  Usage
 *             None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void ringbuf_vInitialize()
{
	/* Init ringbuffer */
    init_ringbuffer(&rb_tx_uart, rb_tx_mempool, UART_TX_RB_LEN);
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
 * Modify[oliver]:
 *       Master Mode:
 *       tx_ringbuffer[data]-->AUPS(here,AUPS is just like a external Co-Processor)
 *       Slave Mode:
 *       tx_ringbuffer[data]-->UART FIFO
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



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
