/*
 * firmware_rpc.c
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


#ifndef TRACE_RPC
#define TRACE_RPC  FALSE
#endif
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <common.h>
#include "zps_apl_aib.h"
#include "firmware_rpc.h"
#include "firmware_algorithm.h"
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void RPC_vHandleDataIndicatorEvent(ZPS_tsAfEvent *sStackEvent);
PRIVATE bool vParseArguments(char* strRequest, tsArguments* tsArg);
PRIVATE char* search_arg(char **arg, char *p, char next_sep);
PRIVATE void RpcHandler(tsArguments *tsArg);


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
		{0xB6B0A1DC, "home_obj1", methodEntityA, 2},
		{0x52560720, "office_obj2", methodEntityB, 2},
		{0x95B96A69, "television_obj3", methodEntityC, 2},
		{0x553EAE82, "washer_obj4", methodEntityD, 2}
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

/****************************************************************************/
/***        Tasks                                                         ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_taskRPC
 *
 * DESCRIPTION:
 * Task to handle to end point(1) events (transmit-related event)
 * RPC server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskRPC)
{
    ZPS_tsAfEvent sStackEvent;

    DBG_vPrintf(TRACE_RPC, "-RPC Thread- \r\n");

    if (g_sDevice.eState != E_NETWORK_RUN)
        return;

    if (OS_eCollectMessage(APP_msgRpcEvents, &sStackEvent) == OS_E_OK)
    {
        if ((ZPS_EVENT_APS_DATA_INDICATION == sStackEvent.eType))
        {
            DBG_vPrintf(TRACE_RPC, "[D_IND] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);

            /* Handle stack event's data from AirPort */
            RPC_vHandleDataIndicatorEvent(&sStackEvent);
        }
        else if (ZPS_EVENT_APS_DATA_CONFIRM == sStackEvent.eType)
        {
            if (g_sDevice.config.txMode == BROADCAST)
            {
                DBG_vPrintf(TRACE_RPC, "[D_CFM] from 0x%04x \r\n",
                            sStackEvent.uEvent.sApsDataConfirmEvent.uDstAddr.u16Addr);
            }
        }
        else if (ZPS_EVENT_APS_DATA_ACK == sStackEvent.eType)
        {
            DBG_vPrintf(TRACE_RPC, "[D_ACK] from 0x%04x \r\n",
                        sStackEvent.uEvent.sApsDataAckEvent.u16DstAddr);
        }
        else
        {
            DBG_vPrintf(TRACE_RPC, "[UNKNOWN] event: 0x%x\r\n", sStackEvent.eType);
        }
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: RPC_vHandleDataIndicatorEvent
 *
 * DESCRIPTION:
 * Handle RPC request and response
 * RPC server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void RPC_vHandleDataIndicatorEvent(ZPS_tsAfEvent *sStackEvent)
{
	PDUM_thAPduInstance hapduInstance;
	uint16 u16PayloadSize;
	char *payload_addr;
	char temp[RPC_MAX_STR_LEN] = {0};

	/* Get data from Stack Event */
	hapduInstance = sStackEvent->uEvent.sApsDataIndEvent.hAPduInst;         //APDU
	u16PayloadSize = PDUM_u16APduInstanceGetPayloadSize(hapduInstance);    //Payload size
	payload_addr = PDUM_pvAPduInstanceGetPayload(hapduInstance);           //Get Payload's address

	/* verify */
	if(u16PayloadSize > RPC_MAX_STR_LEN)
	{
		PDUM_eAPduFreeAPduInstance(hapduInstance);
		return;
	}

    memcpy(temp, payload_addr, u16PayloadSize);

    /* Parse arguments */
	tsArguments args;
	memset((uint8*)&args, 0, sizeof(tsArguments));
	bool ret = vParseArguments(temp, &args);
    if(ret)
    {
		DBG_vPrintf(TRACE_RPC, "obj_name: %s\r\n",args.obj_name);
		DBG_vPrintf(TRACE_RPC, "method_name: %s\r\n",args.method_name);
		DBG_vPrintf(TRACE_RPC, "argc: %d\r\n",args.argc);
		int i=0;
		for(;i<args.argc;i++)
			DBG_vPrintf(TRACE_RPC, "param%d: %s\r\n",i, args.argv[i]);

		/* Handle Rpc request */
		RpcHandler(&args);
    }
    else
    {
    	DBG_vPrintf(TRACE_RPC, "Invalid RPC request format\r\n");
    }

    /* free apdu immediately, without which the RF will crash */
    PDUM_eAPduFreeAPduInstance(hapduInstance);
}

/****************************************************************************
 *
 * NAME: RpcHandler
 *
 * DESCRIPTION:
 * handle Rpc request
 *
 * PARAMETERS: Name         RW  Usage
 *             tsArg        R   arguments from Rpc request
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void RpcHandler(tsArguments *tsArg)
{
    /* Get obj pos */
	int objPos = GetRpcEntityElementPos(tsArg->obj_name, rpcEntity, sizeof(rpcEntity)/sizeof(tsRpcEntity));
    if(-1 == objPos)  return;

    /* get method pos */
    int methodPos = GetMethodEntityElementPos(tsArg->method_name,
    		                      rpcEntity[objPos].methodArray,
    		                      rpcEntity[objPos].methodNum);

    if(-1 == methodPos)  return;

    /* Call the function */
    if(rpcEntity[objPos].methodArray[methodPos].rpcMethod != NULL)
    {
    	rpcEntity[objPos].methodArray[methodPos].rpcMethod(tsArg);
    }
}
/****************************************************************************/
/***        Public Functions                                              ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: RPC_vInit
 *
 * DESCRIPTION:
 * Init the RPC module
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void RPC_vInit(void)
{
    /* calculate hash key */
    int rpcEntityNum = sizeof(rpcEntity)/sizeof(tsRpcEntity);
    int i = 0;
    for(; i < rpcEntityNum; i++)
    {
        rpcEntity[i].hashKey = calc_hashnr(rpcEntity[i].objName, strlen(rpcEntity[i].objName));
        int j = 0;
        for(; j < rpcEntity[i].methodNum; j++)
        {
        	rpcEntity[i].methodArray[j].hashKey = calc_hashnr(rpcEntity[i].methodArray[j].methodName,strlen(rpcEntity[i].methodArray[j].methodName));
        }
    }

}

/****************************************************************************
 *
 * NAME: RPC_vCaller
 *
 * DESCRIPTION:
 * User can use this function to make an Rpc call
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void RPC_vCaller(char* cmd, uint8 port, uint64 mac)
{
	char tmp[RPC_MAX_STR_LEN];
	if(strlen(cmd) + 1 > RPC_MAX_STR_LEN) return;

	strcpy(tmp, cmd);
	API_bSendToMacDev(mac, 2, port, tmp, strlen(tmp) + 1);
}
/****************************************************************************/
/***        Tools Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: search_arg
 *
 * DESCRIPTION:
 * Search arguments
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE char* search_arg(char **arg, char *p, char next_sep)
{
    char *s = p;
    while (true)
    {
        if ((*p == '/') || (*p == ' ') || (*p == '\n') || (*p == '\0')) break;
        p++;
    }
    if (p == s) return NULL;
    *arg = s;
    char separator = *p;
    *p = '\0';
    p++;
    return (separator == next_sep) ? (p) : (NULL);
}

/****************************************************************************
 *
 * NAME: vParseArguments
 *
 * DESCRIPTION:
 * Parse the command string from remote device
 * Commands format:
 * /Obj_Name/Method_Name  param1  param2
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE bool vParseArguments(char* strRequest, tsArguments* tsArg)
{
	/* Clean parameter */
    memset((uint8*)tsArg, 0, sizeof(tsArguments));

	/* Initial '/' */
	char* p = strRequest;
	if (*p != '/') return false;
	p++;

	/* Object Name */
	p = search_arg(&(tsArg->obj_name), p, '/');
	if (p == NULL) return false;

	/* Method Name */
	p = search_arg(&(tsArg->method_name), p, ' ');
	if (p == NULL) return false;

	/* Arguments */
	while (true)
	{
		tsArg->argv[tsArg->argc] = NULL;
		p = search_arg(&(tsArg->argv[tsArg->argc]), p, ' ');
		if (tsArg->argv[tsArg->argc] != NULL)  tsArg->argc++;
		/* modify RPC_MAX_ARGS to meet your require */
		if(p == NULL || tsArg->argc>=RPC_MAX_ARGS) break;
	}

	return true;
}


/****************************************************************************
 *
 * NAME: GetRpcEntityElementPos
 *
 * DESCRIPTION:
 * Get the position of specified elements from hash table
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * position
 *
 ****************************************************************************/
int GetRpcEntityElementPos( char *lpszString, tsRpcEntity *lpTable, int nTableSize )
{
    uint32  nHash = calc_hashnr(lpszString, strlen(lpszString));
    int  nHashStart = nHash % nTableSize;
    int  nHashPos = nHashStart;

    while (true)
    {
		if (lpTable[nHashPos].hashKey == nHash)
		{
			return nHashPos;
		}
		else
		{
			nHashPos = (nHashPos + 1) % nTableSize;
		}

		if (nHashPos == nHashStart)
			break;
    }
     return -1;
}

/****************************************************************************
 *
 * NAME: GetMethodEntityElementPos
 *
 * DESCRIPTION:
 * Get the position of specified elements from hash table
 *
 * PARAMETERS: Name         RW  Usage
 *
 *
 * RETURNS:
 * position
 *
 ****************************************************************************/
int GetMethodEntityElementPos( char *lpszString, tsMethodEntity *lpTable, int nTableSize )
{
    uint32  nHash = calc_hashnr(lpszString, strlen(lpszString));
    int  nHashStart = nHash % nTableSize;
    int  nHashPos = nHashStart;

    while (true)
    {
		if (lpTable[nHashPos].hashKey == nHash)
		{
			return nHashPos;
		}
		else
		{
			nHashPos = (nHashPos + 1) % nTableSize;
		}

		if (nHashPos == nHashStart)
			break;
    }
     return -1;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
