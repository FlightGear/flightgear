
#ifndef PA_MAC_CORE_BLOCKING_H_
#define PA_MAC_CORE_BLOCKING_H_

#include "ringbuffer.h"
#include "portaudio.h"
#include "pa_mac_core_utilities.h"

/*
 * Number of miliseconds to busy wait whil waiting for data in blocking calls.
 */
#define PA_MAC_BLIO_BUSY_WAIT_SLEEP_INTERVAL (5)
/*
 * Define exactly one of these blocking methods
 * PA_MAC_BLIO_MUTEX is not actively maintained.
 */
#define PA_MAC_BLIO_BUSY_WAIT
/*
#define PA_MAC_BLIO_MUTEX
*/

typedef struct {
    RingBuffer inputRingBuffer;
    RingBuffer outputRingBuffer;
    PaSampleFormat inputSampleFormat;
    size_t inputSampleSize;
    PaSampleFormat outputSampleFormat;
    size_t outputSampleSize;

    size_t framesPerBuffer;

    int inChan;
    int outChan;

    //PaStreamCallbackFlags statusFlags;
    uint32_t statusFlags;
    PaError errors;

    /* Here we handle blocking, using condition variables. */
#ifdef  PA_MAC_BLIO_MUTEX
    volatile bool isInputEmpty;
    pthread_mutex_t inputMutex;
    pthread_cond_t inputCond;

    volatile bool isOutputFull;
    pthread_mutex_t outputMutex;
    pthread_cond_t outputCond;
#endif
}
PaMacBlio;

/*
 * These functions operate on condition and related variables.
 */

PaError initializeBlioRingBuffers(
                                       PaMacBlio *blio,
                                       PaSampleFormat inputSampleFormat,
                                       PaSampleFormat outputSampleFormat,
                                       size_t framesPerBuffer,
                                       long ringBufferSize,
                                       int inChan,
                                       int outChan );
PaError destroyBlioRingBuffers( PaMacBlio *blio );
PaError resetBlioRingBuffers( PaMacBlio *blio );

int BlioCallback(
        const void *input, void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData );

void waitUntilBlioWriteBufferIsFlushed( PaMacBlio *blio );

#endif /*PA_MAC_CORE_BLOCKING_H_*/
