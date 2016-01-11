/*
 * firmware_uart.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) NXP B.V. 2012.
 * Spread by SeeedStudio
 * Author     : Oliver Wang
 * Create Time: 2014/06
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

#ifndef RPC_USR_H_
#define RPC_USR_H_
/****************************************************************************/
/***        Macro definitions                                             ***/
/****************************************************************************/
#define METHOD_ENTITY_SIZE(x)  sizeof(x)/sizeof(tsMethodEntity)

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "jendefs.h"
#include "common.h"
#include "jendefs.h"
#include "firmware_rpc.h"

/*  
 * Guide for adding your own RPC functions
 * Only three simple steps are required
 * 1. Add a set of methods which is divided into groups according to their objName, to a methodEntity.
 * 2. Add one obj(something like air_conditioner, or light_switch) to rpcEntity[]
 * 3. Implement these Rpc method. 
*/

/* 
 * [RpcMethod prototypes] 
*/
bool A_run(tsArguments tsArg);
bool A_stop(tsArguments tsArg);
bool B_run(tsArguments tsArg);
bool B_stop(tsArguments tsArg);
bool C_run(tsArguments tsArg);
bool C_stop(tsArguments tsArg);
bool D_run(tsArguments tsArg);
bool D_stop(tsArguments tsArg);


/* hash tree search algorithm for rpc_func search */
/* Step1, MethodEntity: HashKey, MethodName, MethodPointer */
tsMethodEntity methodEntityA[] = {
		{0, "run", A_run},
		{0, "stop", A_stop}
};

tsMethodEntity methodEntityB[] = {
		{0, "run", B_run},
		{0, "stop", B_stop}
};

tsMethodEntity methodEntityC[] = {
		{0, "run", C_run},
		{0, "stop", C_stop}
};

tsMethodEntity methodEntityD[] = {
		{0, "run", D_run},
		{0, "stop", D_stop}
};

/* Step2, Rpc Entity: HashKey, objName, MethodArray, MethodNum */
tsRpcEntity rpcEntity[] = {
		{0, "home_obj1", methodEntityA, METHOD_ENTITY_SIZE(methodEntityA)},
		{0, "office_obj2", methodEntityB, METHOD_ENTITY_SIZE(methodEntityB)},
		{0, "television_obj3", methodEntityC, METHOD_ENTITY_SIZE(methodEntityC)},
		{0, "washer_obj4", methodEntityD, METHOD_ENTITY_SIZE(methodEntityD)}
};

/* Step3, [Rpc Method defined here] */
bool A_run(tsArguments tsArg)
{
    DBG_vPrintf(TRACE_RPC, "home_obj is running \r\n");
    return TRUE;
}

bool A_stop(tsArguments tsArg)
{
	DBG_vPrintf(TRACE_RPC, "home_obj is stopping \r\n");
    return TRUE;
}

bool B_run(tsArguments tsArg)
{
	DBG_vPrintf(TRACE_RPC, "office_obj2 is running \r\n");
    return TRUE;
}

bool B_stop(tsArguments tsArg)
{
	DBG_vPrintf(TRACE_RPC, "office_obj2 is stopping \r\n");
    return TRUE;
}

bool C_run(tsArguments tsArg)
{

    return TRUE;
}

bool C_stop(tsArguments tsArg)
{

    return TRUE;
}

bool D_run(tsArguments tsArg)
{

    return TRUE;
}

bool D_stop(tsArguments tsArg)
{

    return TRUE;
}
#endif /* RPC_USR_H_ */
