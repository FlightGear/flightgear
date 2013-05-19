/*
 * PortMixer
 * Mac OS 9 implementation
 *
 * Copyright (c) 2002
 *
 * Written by Dominic Mazzoni
 *
 * PortMixer is intended to work side-by-side with PortAudio,
 * the Portable Real-Time Audio Library by Ross Bencina and
 * Phil Burk.
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
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "portaudio.h"
#include "pa_host.h"
#include "portmixer.h"

#define PA_MAX_NUM_HOST_BUFFERS           (16)   /* Do not exceed!! */

typedef struct MultiBuffer
{
    char    *buffers[PA_MAX_NUM_HOST_BUFFERS];
    int      numBuffers;
    int      nextWrite;
    int      nextRead;
}
MultiBuffer;

/* Define structure to contain all Macintosh specific data. */
typedef struct PaHostSoundControl
{
    UInt64                  pahsc_EntryCount;
	double                  pahsc_InverseMicrosPerHostBuffer; /* 1/Microseconds of real-time audio per user buffer. */

    /* Use char instead of Boolean for atomic operation. */
    volatile char           pahsc_IsRecording;   /* Recording in progress. Set by foreground. Cleared by background. */
    volatile char           pahsc_StopRecording; /* Signal sent to background. */
    volatile char           pahsc_IfInsideCallback;
    /* Input */
    SPB                     pahsc_InputParams;
    SICompletionUPP         pahsc_InputCompletionProc;
    MultiBuffer             pahsc_InputMultiBuffer;
    int32                   pahsc_BytesPerInputHostBuffer;
    int32                   pahsc_InputRefNum;
    /* Output */
    CmpSoundHeader          pahsc_SoundHeaders[PA_MAX_NUM_HOST_BUFFERS];
    int32                   pahsc_BytesPerOutputHostBuffer;
    SndChannelPtr           pahsc_Channel;
    SndCallBackUPP          pahsc_OutputCompletionProc;
    int32                   pahsc_NumOutsQueued;
    int32                   pahsc_NumOutsPlayed;
    PaTimestamp             pahsc_NumFramesDone;
    UInt64                  pahsc_WhenFramesDoneIncremented;
    /* Init Time -------------- */
    int32                   pahsc_NumHostBuffers;
    int32                   pahsc_FramesPerHostBuffer;
    int32                   pahsc_UserBuffersPerHostBuffer;
    int32                   pahsc_MinFramesPerHostBuffer; /* Can vary depending on virtual memory usage. */
}
PaHostSoundControl;

typedef struct PxSource
{
   char name[256];
} PxSource;

typedef struct PxInfo
{
   SPB           *input;
   int32          inputRefNum;
   SndChannelPtr  output;
   int32          numSources;
   PxSource      *sources;
} PxInfo;

int Px_GetNumMixers( void *pa_stream )
{
   return 0;
}

const char *Px_GetMixerName( void *pa_stream, int index )
{
   return "Mac Sound Manager";
}

PxMixer *Px_OpenMixer( void *pa_stream, int index )
{
   PxInfo                      *info;
   internalPortAudioStream     *past;
   PaHostSoundControl          *macInfo;
   OSErr                        err;
   int                          i, j;
   Handle                       h;
   unsigned char               *data;
   
   info = (PxInfo *)malloc(sizeof(PxInfo));   
   past = (internalPortAudioStream *) pa_stream;
   macInfo = (PaHostSoundControl *) past->past_DeviceData;

   info->input = &macInfo->pahsc_InputParams;
   info->inputRefNum = macInfo->pahsc_InputRefNum;
   info->output = macInfo->pahsc_Channel;

   info->numSources = 0;
   info->sources = NULL;
   
   err = SPBGetDeviceInfo (info->inputRefNum, siInputSourceNames, &h);
   if (err)
      return (PxMixer *)info;
   
   HLock(h);
   HNoPurge(h);
   
   data = (unsigned char *)*h;
   info->numSources = ((short *)data)[0];
   if (info->numSources <= 0 || info->numSources > 50) {
      HUnlock(h);
      return (PxMixer *)info;
   }
   
   info->sources = (PxSource *)malloc(info->numSources * sizeof(PxSource));
   data += 2;
   for(i=0; i<info->numSources; i++) {
      int len = *data++;
      
      if (len > 63) {
         info->numSources = 0;
         free(info->sources);
         info->sources = NULL;
         HUnlock(h);
         return (PxMixer *)info;
      }

      for(j=0; j<len; j++)
         info->sources[i].name[j] = *data++;

      info->sources[i].name[len] = 0;
   }
   HUnlock(h);
   
   return (PxMixer *)info;
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

void Px_CloseMixer(PxMixer *mixer)
{
   PxInfo *info = (PxInfo *)mixer;
   
   if (info->sources)
      free(info->sources);
   free(info);
}

/*
 Master (output) volume
*/

PxVolume Px_GetMasterVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return 0.0;
}

void Px_SetMasterVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
}

/*
 PCM output volume
*/

int Px_SupportsPCMOutputVolume( PxMixer* mixer ) 
{
	return 1 ; 
}

PxVolume Px_GetPCMOutputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   OSErr err;
   long packedVol;
   SndCommand cmd;

   cmd.cmd = getVolumeCmd;
   cmd.param1 = 0;
   cmd.param2 = (long)&packedVol;
   
   err = SndDoImmediate(info->output, &cmd);
   if (err)
      return 0.0;
   
   return ((packedVol & 0xFFFF) + ((packedVol & 0xFFFF0000) >> 16) / 2.0) / 256.0;
}

void Px_SetPCMOutputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   OSErr err;
   long packedVol;
   SndCommand cmd;

   packedVol = (unsigned long)volume * 256.0;
   packedVol += (packedVol << 16);
   
   cmd.cmd = volumeCmd;
   cmd.param1 = 0;
   cmd.param2 = packedVol;
   err = SndDoImmediate(info->output, &cmd);
}

/*
 All output volumes
*/

int Px_GetNumOutputVolumes( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return 0;
}

const char *Px_GetOutputVolumeName( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   
   return NULL;
}

PxVolume Px_GetOutputVolume( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;

   return NULL;
}

void Px_SetOutputVolume( PxMixer *mixer, int i, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
}

/*
 Input sources
*/

int Px_GetNumInputSources( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   
   return info->numSources;
}

const char *Px_GetInputSourceName( PxMixer *mixer, int i)
{
   PxInfo *info = (PxInfo *)mixer;
   
   if (i >= 0 && i < info->numSources)
      return info->sources[i].name;
   else
      return "";
}

int Px_GetCurrentInputSource( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   short selected;
   OSErr err;

   err = SPBGetDeviceInfo (info->inputRefNum, siInputSource, &selected);
   if (err)
      return 0;

   return selected - 1;
}

void Px_SetCurrentInputSource( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   short selected = i+1;
   OSErr err;
   
   err = SPBSetDeviceInfo (info->inputRefNum, siInputSource, &selected);
}

/*
 Input volume
*/

PxVolume Px_GetInputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   Fixed fixedGain;
   PxVolume vol;
   OSErr err;
   
   if (info->input) {
      err = SPBGetDeviceInfo(info->inputRefNum, siInputGain, (Ptr)&fixedGain);
      if (err)
         return 0.0;
      
      vol = (fixedGain / 65536.0) - 0.5;
      return vol;
   }

   return 0.0;
}

void Px_SetInputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   Fixed fixedGain;
   OSErr err;
   
   if (info->input) {
      fixedGain = (Fixed)((volume + 0.5) * 65536.0);
      err = SPBSetDeviceInfo(info->inputRefNum, siInputGain, (Ptr)&fixedGain);
   }
}

/*
  Balance
*/

int Px_SupportsOutputBalance( PxMixer *mixer )
{
   return 0;
}

PxBalance Px_GetOutputBalance( PxMixer *mixer )
{
   return 0.0;
}

void Px_SetOutputBalance( PxMixer *mixer, PxBalance balance )
{
}

/*
  Playthrough
*/

int Px_SupportsPlaythrough( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return (info->input != NULL);
}

PxVolume Px_GetPlaythrough( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   OSErr err;
   short level;

   err = SPBGetDeviceInfo(info->inputRefNum, siPlayThruOnOff, (Ptr)&level);
   if (err)
      return 0.0;

   if (level < 0)
      level = 0;
   if (level > 7)
      level = 7;
   
   return (PxVolume)(level / 7.0);
}

void Px_SetPlaythrough( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   OSErr err;
   short level = (int)(volume * 7.0 + 0.5);

   err = SPBSetDeviceInfo(info->inputRefNum, siPlayThruOnOff, (Ptr)&level);
}
