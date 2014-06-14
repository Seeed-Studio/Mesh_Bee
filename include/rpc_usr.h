/*
 * rpc_usr.h
 *
 *  Created on: 2014-6-14
 *      Author: Administrator
 */

#ifndef RPC_USR_H_
#define RPC_USR_H_

#define METHOD_ENTITY_SIZE(x)  sizeof(x)/sizeof(tsMethodEntity)


#include "jendefs.h"

#include "rpc_usr.h"
#include "common.h"
#include "jendefs.h"
#include "firmware_rpc.h"

/* [RpcMethod prototypes] */
bool A_run(tsArguments tsArg);
bool A_stop(tsArguments tsArg);
bool B_run(tsArguments tsArg);
bool B_stop(tsArguments tsArg);
bool C_run(tsArguments tsArg);
bool C_stop(tsArguments tsArg);
bool D_run(tsArguments tsArg);
bool D_stop(tsArguments tsArg);





/* hash tree search algorithm for rpc_func search */
/* MethodEntity: HashKey, MethodName, MethodPointer */
tsMethodEntity methodEntityA[] = {
		{0x7A19CDD7, "run", A_run},
		{0x5BC1D108, "stop", A_stop}
};

tsMethodEntity methodEntityB[] = {
		{0x7A19CDD7, "run", B_run},
		{0x5BC1D108, "stop", B_stop}
};

tsMethodEntity methodEntityC[] = {
		{0x7A19CDD7, "run", C_run},
		{0x5BC1D108, "stop", C_stop}
};

tsMethodEntity methodEntityD[] = {
		{0x7A19CDD7, "run", D_run},
		{0x5BC1D108, "stop", D_stop}
};

/* Rpc Entity: HashKey, objName, MethodArray, MethodNum */
tsRpcEntity rpcEntity[] = {
		{0xB6B0A1DC, "home_obj1", methodEntityA, METHOD_ENTITY_SIZE(methodEntityA)},
		{0x52560720, "office_obj2", methodEntityB, METHOD_ENTITY_SIZE(methodEntityB)},
		{0x95B96A69, "television_obj3", methodEntityC, METHOD_ENTITY_SIZE(methodEntityC)},
		{0x553EAE82, "washer_obj4", methodEntityD, METHOD_ENTITY_SIZE(methodEntityD)}
};

/* [Rpc Method defined here] */
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
