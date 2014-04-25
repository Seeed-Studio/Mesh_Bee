/*
 * firmware_api_codec.c
 * API codec library for MCU mode(arduino-ful MCU mode)
 *
 * Copyright (c) Seeed Studio. 2014.
 * Author     : Oliver Wang
 * Create Time: 2014/04
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


#include "firmware_api_codec.h"

/****************************************************************************
 *
 * NAME: u16DecodeApiSpec
 *
 * DESCRIPTION:
 * length = cmdData  [delimiter length apiIdentifier cmdData checkSum]
 * Pay attention: 4 bytes align
 * RETURNS:
 * position of start delimiter
 *
 ****************************************************************************/
uint16 u16DecodeApiSpec(uint8 *buffer, int len, tsApiSpec *spec, bool *valid)
{
	uint8 *ptr = buffer;
	while (*ptr != API_START_DELIMITER && len-- > 0) ptr++;
	if (len < 4)
	{
	    *valid = FALSE;
	    return ptr - buffer;
	}

	/* read startDelimiter/length/apiIdentifier */
	memcpy((uint8*)spec, ptr, 3);    //4 bytes align,read 8 bytes

    ptr += 3;
    len -= 3;
    if (len < (spec->length + 1))
    {
        *valid = FALSE;
        return ptr - buffer;
    }

    /* read payload */
    memcpy((uint8*)spec + 3, ptr, spec->length);
    ptr += spec->length;

    /* read checkSum */
    spec->checkSum = *ptr;
    ptr++;

    /* verify checkSum */
    if (calCheckSum((uint8*)spec+3,spec->length) == spec->checkSum)
    {
        *valid = TRUE;
    }
    else
    {
        *valid = FALSE;
    }
    return ptr-buffer;
}
