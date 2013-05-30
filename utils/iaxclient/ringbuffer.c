/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * <see below>
 * Peter Grayson <jpgrayson@gmail.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

/*
 * Ring Buffer utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 * modified for SMP safety on Mac OS X by Bjorn Roche
 * modified for SMP safety on Linux by Leland Lucius
 * also, allowed for const where possible
 * Note that this is safe only for a single-thread reader and a
 * single-thread writer.
 *
 * Originally distributed with the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

/**
 @file
 @ingroup common_src
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "ringbuffer.h"

/****************
 * First, we'll define some memory barrier primitives based on the system.
 * right now only OS X, FreeBSD, and Linux are supported. In addition to providing
 * memory barriers, these functions should ensure that data cached in registers
 * is written out to cache where it can be snooped by other CPUs. (ie, the volatile
 * keyword should not be required)
 *
 * the primitives that must be defined are:
 *
 * rb_FullMemoryBarrier()
 * rb_ReadMemoryBarrier()
 * rb_WriteMemoryBarrier()
 *
 ****************/

/* OSMemoryBarrier() is not portable to Mac OS X 10.3.9. We choose to do
 * the more portable, less correct thing here and just use assembly with
 * known bugs. The downside is that there may be some audio playback
 * issues on some Macs.
 */
#if defined(__APPLE__) && 0
#   include <libkern/OSAtomic.h>
    /* Here are the memory barrier functions. Mac OS X only provides
       full memory barriers, so the three types of barriers are the same. */
#   define rb_FullMemoryBarrier()  OSMemoryBarrier()
#   define rb_ReadMemoryBarrier()  OSMemoryBarrier()
#   define rb_WriteMemoryBarrier() OSMemoryBarrier()
#elif defined(__GNUC__)
    /* GCC understands volatile asm and "memory" to mean it
     * should not reorder memory read/writes */
#   if defined( __ppc__ ) || defined( __powerpc__ )
#      define rb_FullMemoryBarrier()  asm volatile("sync":::"memory")
#      define rb_ReadMemoryBarrier()  asm volatile("sync":::"memory")
#      define rb_WriteMemoryBarrier() asm volatile("sync":::"memory")
#   elif defined( __SSE2__ )
#      define rb_FullMemoryBarrier()  asm volatile("mfence":::"memory")
#      define rb_ReadMemoryBarrier()  asm volatile("lfence":::"memory")
#      define rb_WriteMemoryBarrier() asm volatile("sfence":::"memory")
#   elif defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) || defined( __i686__ )
#      define DO_X86_RUNTIME_CPU_DETECTION
#      define rb_FullMemoryBarrier()  if ( have_sse2 ) asm volatile("mfence":::"memory")
#      define rb_ReadMemoryBarrier()  if ( have_sse2 ) asm volatile("lfence":::"memory")
#      define rb_WriteMemoryBarrier() if ( have_sse2 ) asm volatile("sfence":::"memory")
#   else
#      ifdef ALLOW_SMP_DANGERS
#         warning Memory barriers not defined on this system or system unknown
#         warning For SMP safety, you should fix this.
#         define rb_FullMemoryBarrier()
#         define rb_ReadMemoryBarrier()
#         define rb_WriteMemoryBarrier()
#      else
#         error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#      endif
#   endif
#elif defined(_MSC_VER)
#   if ( _MSC_VER > 1200 )
#      include <intrin.h>
#      pragma intrinsic(_ReadWriteBarrier)
#      pragma intrinsic(_ReadBarrier)
#      pragma intrinsic(_WriteBarrier)
#      define rb_FullMemoryBarrier()  _ReadWriteBarrier()
#      define rb_ReadMemoryBarrier()  _ReadBarrier()
#      define rb_WriteMemoryBarrier() _WriteBarrier()
#   else
#      ifdef ALLOW_SMP_DANGERS
#         pragma message("Memory barriers not defined on this system or system unknown")
#         pragma message("For SMP safety, you should fix this.")
#         define rb_FullMemoryBarrier()
#         define rb_ReadMemoryBarrier()
#         define rb_WriteMemoryBarrier()
#      else
#         error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#      endif
#   endif
#else
#   ifdef ALLOW_SMP_DANGERS
#      warning Memory barriers not defined on this system or system unknown
#      warning For SMP safety, you should fix this.
#      define rb_FullMemoryBarrier()
#      define rb_ReadMemoryBarrier()
#      define rb_WriteMemoryBarrier()
#   else
#      error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#   endif
#endif

#ifdef DO_X86_RUNTIME_CPU_DETECTION
#define cpuid(func, ax, bx, cx, dx)			\
	__asm__ __volatile__ ("xchgl\t%%ebx, %1\n\t"	\
	   "cpuid\n\t"					\
	   "xchgl\t%%ebx, %1\n\t"			\
	   : "=a" (ax), "=r" (bx), "=c" (cx), "=d" (dx)	\
	   : "0" (func))

static int have_sse2 = 0;
#endif

/***************************************************************************
 * Initialize FIFO.
 * numBytes must be power of 2, returns -1 if not.
 */
long rb_InitializeRingBuffer( rb_RingBuffer *rbuf, long numBytes, void *dataPtr )
{
#ifdef DO_X86_RUNTIME_CPU_DETECTION
    /* See http://softpixel.com/~cwright/programming/simd/cpuid.php
     * for a good description of how all this works.
     */
    int a, b, c, d;
    cpuid(1, a, b, c, d);
    have_sse2 = d & (1 << 26);
#endif

    if( ((numBytes-1) & numBytes) != 0) return -1; /* Not Power of two. */
    rbuf->bufferSize = numBytes;
    rbuf->buffer = (char *)dataPtr;
    rb_FlushRingBuffer( rbuf );
    rbuf->bigMask = (numBytes*2)-1;
    rbuf->smallMask = (numBytes)-1;
    return 0;
}

/***************************************************************************
** Return number of bytes available for reading. */
long rb_GetRingBufferReadAvailable( rb_RingBuffer *rbuf )
{
    rb_ReadMemoryBarrier();
    return ( (rbuf->writeIndex - rbuf->readIndex) & rbuf->bigMask );
}
/***************************************************************************
** Return number of bytes available for writing. */
long rb_GetRingBufferWriteAvailable( rb_RingBuffer *rbuf )
{
    /* Since we are calling rb_GetRingBufferReadAvailable, we don't need an aditional MB */
    return ( rbuf->bufferSize - rb_GetRingBufferReadAvailable(rbuf));
}

/***************************************************************************
** Clear buffer. Should only be called when buffer is NOT being read. */
void rb_FlushRingBuffer( rb_RingBuffer *rbuf )
{
    rbuf->writeIndex = rbuf->readIndex = 0;
}

/***************************************************************************
** Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long rb_GetRingBufferWriteRegions( rb_RingBuffer *rbuf, long numBytes,
                                       void **dataPtr1, long *sizePtr1,
                                       void **dataPtr2, long *sizePtr2 )
{
    long   index;
    long   available = rb_GetRingBufferWriteAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if write is not contiguous. */
    index = rbuf->writeIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long   firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}


/***************************************************************************
*/
long rb_AdvanceRingBufferWriteIndex( rb_RingBuffer *rbuf, long numBytes )
{
    /* we need to ensure that previous writes are seen before we update the write index */
    rb_WriteMemoryBarrier();
    return rbuf->writeIndex = (rbuf->writeIndex + numBytes) & rbuf->bigMask;
}

/***************************************************************************
** Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long rb_GetRingBufferReadRegions( rb_RingBuffer *rbuf, long numBytes,
                                void **dataPtr1, long *sizePtr1,
                                void **dataPtr2, long *sizePtr2 )
{
    long   index;
    long   available = rb_GetRingBufferReadAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if read is not contiguous. */
    index = rbuf->readIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}
/***************************************************************************
*/
long rb_AdvanceRingBufferReadIndex( rb_RingBuffer *rbuf, long numBytes )
{
    /* we need to ensure that previous writes are always seen before updating the index. */
    rb_WriteMemoryBarrier();
    return rbuf->readIndex = (rbuf->readIndex + numBytes) & rbuf->bigMask;
}

/***************************************************************************
** Return bytes written. */
long rb_WriteRingBuffer( rb_RingBuffer *rbuf, const void *data, long numBytes )
{
    long size1, size2, numWritten;
    void *data1, *data2;
    numWritten = rb_GetRingBufferWriteRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {

        memcpy( data1, data, size1 );
        data = ((char *)data) + size1;
        memcpy( data2, data, size2 );
    }
    else
    {
        memcpy( data1, data, size1 );
    }
    rb_AdvanceRingBufferWriteIndex( rbuf, numWritten );
    return numWritten;
}

/***************************************************************************
** Return bytes read. */
long rb_ReadRingBuffer( rb_RingBuffer *rbuf, void *data, long numBytes )
{
    long size1, size2, numRead;
    void *data1, *data2;
    numRead = rb_GetRingBufferReadRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        memcpy( data, data1, size1 );
        data = ((char *)data) + size1;
        memcpy( data, data2, size2 );
    }
    else
    {
        memcpy( data, data1, size1 );
    }
    rb_AdvanceRingBufferReadIndex( rbuf, numRead );
    return numRead;
}
