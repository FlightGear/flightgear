/*
 * PortMixer
 * Mac OS X / CoreAudio implementation
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

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioConverter.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>

#include <portaudio.h>
#if 0
#include <pa_mac_core.h>
#else
/* TODO: Fix this when portaudio gets their hostapi-specific stuff
 * straightened up.
 */
#include <AudioUnit/AudioUnit.h>
AudioDeviceID PaMacCore_GetStreamInputDevice( PaStream* s );
AudioDeviceID PaMacCore_GetStreamOutputDevice( PaStream* s );
#endif

#include "portmixer.h"

// define value of isInput passed to CoreAudio routines
#define IS_INPUT    (true)
#define IS_OUTPUT   (false)

typedef struct PxInfo
{
   AudioDeviceID   input;
   AudioDeviceID   output;
} PxInfo;

int Px_GetNumMixers( void *pa_stream )
{
   return 1;
}

const char *Px_GetMixerName( void *pa_stream, int index )
{
   return "CoreAudio";
}

PxMixer *Px_OpenMixer( void *pa_stream, int index )
{
   PxInfo                      *info;
   
   info = (PxInfo *)malloc(sizeof(PxInfo));   
   if (!info) {
      return (PxMixer *)info;
   }

   info->input = PaMacCore_GetStreamInputDevice(pa_stream);
   info->output = PaMacCore_GetStreamOutputDevice(pa_stream);

   return (PxMixer *)info;
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

void Px_CloseMixer(PxMixer *mixer)
{
   PxInfo *info = (PxInfo *)mixer;

   free(info);
}

/*
 Master (output) volume
*/

PxVolume Px_GetMasterVolume( PxMixer *mixer )
{
   return 0.0;
}

void Px_SetMasterVolume( PxMixer *mixer, PxVolume volume )
{
}

/*
 PCM output volume
*/

static PxVolume Px_GetVolume(AudioDeviceID device, Boolean isInput)
{
   OSStatus err;
   UInt32   outSize;
   Float32  vol, maxvol=0.0;
   UInt32   mute, anymuted=0;
   int ch;

   for(ch=0; ch<=2; ch++) {
      outSize = sizeof(Float32);
      err = AudioDeviceGetProperty(device, ch, isInput,
                                   kAudioDevicePropertyVolumeScalar,
                                   &outSize, &vol);
      if (!err) {
         if (vol > maxvol)
            maxvol = vol;
      }

      outSize = sizeof(UInt32);
      err = AudioDeviceGetProperty(device, ch, isInput,
                                   kAudioDevicePropertyMute,
                                   &outSize, &mute);

      if (!err) {
         if (mute)
            anymuted = 1;
      }
   }

   if (anymuted)
      maxvol = 0.0;

   return maxvol;
}

static void Px_SetVolume(AudioDeviceID device, Boolean isInput,
                         PxVolume volume)
{
   Float32  vol = volume;
   UInt32 mute = 0;
   int ch;
   OSStatus err;

   /* Implement a passive attitude towards muting.  If they
      drag the volume above 0.05, unmute it.  But if they
      drag the volume down below that, just set the volume,
      don't actually mute.
   */

   for(ch=0; ch<=2; ch++) {
      err =  AudioDeviceSetProperty(device, 0, ch, isInput,
                                    kAudioDevicePropertyVolumeScalar,
                                    sizeof(Float32), &vol);
      if (vol > 0.05) {
         err =  AudioDeviceSetProperty(device, 0, ch, isInput,
                                       kAudioDevicePropertyMute,
                                       sizeof(UInt32), &mute);
      }
   }
}

int Px_SupportsPCMOutputVolume( PxMixer* mixer ) 
{
	return 1 ;
}

PxVolume Px_GetPCMOutputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return Px_GetVolume(info->output, IS_OUTPUT);
}

void Px_SetPCMOutputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   Px_SetVolume(info->output, IS_OUTPUT, volume);
}

/*
 All output volumes
*/

int Px_GetNumOutputVolumes( PxMixer *mixer )
{
   return 1;
}

const char *Px_GetOutputVolumeName( PxMixer *mixer, int i )
{
   if (i == 0)
      return "PCM";
   else
      return "";
}

PxVolume Px_GetOutputVolume( PxMixer *mixer, int i )
{
   return Px_GetPCMOutputVolume(mixer);
}

void Px_SetOutputVolume( PxMixer *mixer, int i, PxVolume volume )
{
   Px_SetPCMOutputVolume(mixer, volume);
}

/*
 Input sources
*/

int Px_GetNumInputSources( PxMixer *mixer )
{
   return 1 ;
}

const char *Px_GetInputSourceName( PxMixer *mixer, int i)
{
   return "Default Input Source" ;
}

int Px_GetCurrentInputSource( PxMixer *mixer )
{
   return -1; /* none */
}

void Px_SetCurrentInputSource( PxMixer *mixer, int i )
{
}

/*
 Input volume
*/

PxVolume Px_GetInputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return Px_GetVolume(info->input, IS_INPUT);
}

void Px_SetInputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   Px_SetVolume(info->input, IS_INPUT, volume);
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
   return 1;
}

PxVolume Px_GetPlaythrough( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   OSStatus err;
   UInt32   outSize;
   UInt32   flag;

   outSize = sizeof(UInt32);
   err =  AudioDeviceGetProperty(info->output, 0, IS_OUTPUT,
                                 kAudioDevicePropertyPlayThru,
                                 &outSize, &flag);
   if (err)
      return 0.0;
 
   if (flag)
      return 1.0;
   else
      return 0.0;
}

void Px_SetPlaythrough( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   UInt32 flag = (volume > 0.01);
   OSStatus err;

   err =  AudioDeviceSetProperty(info->output, 0, 0, IS_OUTPUT,
                                 kAudioDevicePropertyPlayThru,
                                 sizeof(UInt32), &flag);
}

/*
  unimplemented stubs
*/

int Px_SetMicrophoneBoost( PxMixer* mixer, int enable )
{
	return 1 ;
}

int Px_GetMicrophoneBoost( PxMixer* mixer )
{
	return -1 ;
}

int Px_SetCurrentInputSourceByName( PxMixer* mixer, const char* line_name )
{
	return 1 ;
}


