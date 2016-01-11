/*
 * firmware_algorithm.c
 * Firmware for SeeedStudio Mesh Bee(Zigbee) module
 *
 * Copyright (c) Seeed Inc 2014.
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
/* size 0x500 is recommended, but device lacks of memory now */
//volatile static uint32 cryptTable[1] = {0};
//
//void prepareCryptTable()
//{
//	uint32 seed = 0x00100001, index1 = 0, index2 = 0, i;
//    for( index1 = 0; index1 < 0x100; index1++ )
//    {
//    	for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
//        {
//            uint32 temp1, temp2;
//            seed = (seed * 125 + 3) % 0x2AAAAB;
//            temp1 = (seed & 0xFFFF) << 0x10;
//            seed = (seed * 125 + 3) % 0x2AAAAB;
//            temp2 = (seed & 0xFFFF);
//            cryptTable[index2] = ( temp1 | temp2 );
//       }
//   }
//}
//
///* [Algorithm 0-1] hash */
//uint32 HashString(char *lpszFileName, uint32 dwHashType)
//{
//	uint8 *key = (uint8 *)lpszFileName;
//	uint32 seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
//	int ch;
//
//	while(*key != 0)
//	{
//		ch = toupper(*key++);
//		seed1 = cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2);
//		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
//	}
//	return seed1;
//}
//
//int GetHashTablePos( char *lpszString, void *lpTable, int nTableSize )
//{
//    const int  HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;
//
//    int  nHash = HashString( lpszString, HASH_OFFSET );
//    int  nHashA = HashString( lpszString, HASH_A );
//    int  nHashB = HashString( lpszString, HASH_B );
//    int  nHashStart = nHash % nTableSize;
//    int  nHashPos = nHashStart;
//
//    while ( lpTable[nHashPos].bExists )
//    {
//    if (lpTable[nHashPos].nHashA == nHashA && lpTable[nHashPos].nHashB == nHashB)
//    {
//      return nHashPos;
//    }
//    else
//    {
//       nHashPos = (nHashPos + 1) % nTableSize;
//    }
//
//    if (nHashPos == nHashStart)
//        break;
//    }
//     return -1;
//}

/*
 * [Algorithm 0-2] hash
 * works well on both numbers and strings
 * The magic is in the interesting relationship between the special prime
 * 16777619 (2^24 + 403) and 2^32 and 2^8.
 * key: input string, len: length of the key
*/
PUBLIC uint32 calc_hashnr(const char *key, uint32 len)
{
    const char *end = key + len;
    uint32 hash;
    for(hash = 0; key < end; key++)
    {
        hash *= 16777619;
        hash ^= (uint32)*(unsigned char*)key;
    }
    return (hash);
}

//int GetElementPos( char *lpszString, void *lpTable, int nTableSize )
//{
//    uint32  nHash = calc_hashnr(lpszString, strlen(lpszString));
//    int  nHashStart = nHash % nTableSize;
//    int  nHashPos = nHashStart;
//
//    while (true)
//    {
//		if (lpTable[nHashPos].nHash == nHash)
//		{
//			return nHashPos;
//		}
//		else
//		{
//			nHashPos = (nHashPos + 1) % nTableSize;
//		}
//
//		if (nHashPos == nHashStart)
//			break;
//    }
//     return -1;
//}


