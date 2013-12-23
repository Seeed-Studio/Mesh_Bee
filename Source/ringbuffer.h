/*    
 * ringbuffer.h
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
#ifndef TERMBOX_RINGBUFFER_H
#define TERMBOX_RINGBUFFER_H

#include <jendefs.h>

#define ERINGBUFFER_ALLOC_FAIL -1

struct ringbuffer {
    char *buf;
    uint32 size;

    char *begin;
    char *end;
};

typedef struct ringbuffer RingBuffer;

int init_ringbuffer(struct ringbuffer *r, void *buff, uint32 size);
void free_ringbuffer(struct ringbuffer *r);
void clear_ringbuffer(struct ringbuffer *r);
uint32 ringbuffer_free_space(struct ringbuffer *r);
uint32 ringbuffer_data_size(struct ringbuffer *r);
void ringbuffer_push(struct ringbuffer *r, const void *data, uint32 size);
void ringbuffer_pop(struct ringbuffer *r, void *data, uint32 size);
void ringbuffer_read(struct ringbuffer *r, void *data, uint32 size);

#endif
