/*
 * firmware_ups.c
 * - User Programming Space -
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 * 
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Oliver Wang
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
#include "common.h"
#include "suli.h"
#include "ups_arduino_sketch.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_UPS
#define TRACE_UPS TRUE
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
PRIVATE uint32 _loopInterval = 0;

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: ups_init
 *
 * DESCRIPTION:
 * init user programming space
 *
 * PARAMETERS: Name         RW  Usage
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void ups_init(void)
{
	//init suli
    suli_init();
    //init arduino sketch with arduino-style setup function
    arduino_setup();
    //start arduino loops, Arduino_LoopTimer is bound with Arduino_Loop task
    OS_eStartSWTimer(Arduino_LoopTimer, APP_TIME_MS(1), NULL);
}

/****************************************************************************
 *
 * NAME: setLoopInterval
 *
 * DESCRIPTION:
 * set the interval between loops
 *
 * PARAMETERS: Name         RW  Usage
 *             ms           W   interval in ms
 *
 * RETURNS:
 * void
 * 
 ****************************************************************************/
void setLoopIntervalMs(uint32 ms)
{
    _loopInterval = ms;
}


/****************************************************************************
 *
 * NAME: Arduino_Loop
 *
 * DESCRIPTION:
 * task for arduino loop
 *
 ****************************************************************************/
OS_TASK(Arduino_Loop)
{
    arduino_loop();
    if(_loopInterval > 0)
    {
		OS_eStartSWTimer(Arduino_LoopTimer, APP_TIME_MS(_loopInterval), NULL); 
    } else
    {
		OS_eActivateTask(Arduino_Loop); 
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

