/*
 * PortMixer
 * Empty implementation (as a placeholder on platforms where
 * it hasn't been implemented yet)
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

#include "portaudio.h"
#include "portmixer.h"

typedef struct PxInfo
{
  int dummy;
} PxInfo;

int Px_GetNumMixers( void *pa_stream )
{
   return 0;
}

const char *Px_GetMixerName( void *pa_stream, int index )
{
   return NULL;
}

PxMixer *Px_OpenMixer( void *pa_stream, int index )
{
   return NULL;
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

void Px_CloseMixer(PxMixer *mixer)
{
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

PxVolume Px_GetPCMOutputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return 0.0;
}

void Px_SetPCMOutputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
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

   return 0;
}

const char *Px_GetInputSourceName( PxMixer *mixer, int i)
{
   PxInfo *info = (PxInfo *)mixer;

   return NULL;
}

int Px_GetCurrentInputSource( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return -1; /* none */
}

void Px_SetCurrentInputSource( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
}

/*
 Input volume
*/

PxVolume Px_GetInputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return 0.0;
}

void Px_SetInputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
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
   return 0;
}

PxBalance Px_GetPlaythrough( PxMixer *mixer )
{
   return 0.0;
}

void Px_SetPlaythrough( PxMixer *mixer, PxBalance balance )
{
}

