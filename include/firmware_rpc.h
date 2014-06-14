/*
 * firmware_rpc.h
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) Seeed Inc. 2014.
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

#ifndef FIRMWARE_RPC_H_
#define FIRMWARE_RPC_H_
#include <jendefs.h>

/* [Macro Define] */
#define  RPC_MAX_ARGS          2     //this is the max arg number
#define  RPC_MAX_OBJ_NAME      8
#define  RPC_MAX_METHOD_NAME   8
#define  RPC_MAX_STR_LEN       30

enum ePcktType
{
    RPC_REQ = 1,
    PRC_RESP = 2
};

/* Arguments of Rpc method */
typedef struct
{
	char *obj_name;
	char *method_name;
    int argc;
    char *argv[RPC_MAX_ARGS];
}tsArguments;

typedef bool (*RPC_Method_t)(tsArguments*);

/* Rpc method entity */
typedef struct
{
	uint32 hashKey;           //method_name's hash key
    char *methodName;         //method_name string
    RPC_Method_t rpcMethod;   //pointer of Rpc function
}tsMethodEntity;

/* Rpc entity */
typedef struct _rpcEntity
{
	uint32 hashKey;              //objName's hash key
	char *objName;               //objName string
	tsMethodEntity *methodArray; //pointer to group method list
	uint8 methodNum;             //numbers of method in this group
}tsRpcEntity;




/* [function declaration] */
int GetRpcEntityElementPos( char *lpszString, tsRpcEntity *lpTable, int nTableSize );
int GetMethodEntityElementPos( char *lpszString, tsMethodEntity *lpTable, int nTableSize );
PUBLIC void RPC_vInit(void);
PUBLIC void RPC_vCaller(char* cmd, uint8 port, uint64 mac);
#endif /* FIRMWARE_RPC_H_ */
