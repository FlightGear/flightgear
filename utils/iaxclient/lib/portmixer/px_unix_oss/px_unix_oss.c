/*
 * PortMixer
 * Unix OSS Implementation
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

#if defined(__linux__)
#include <linux/soundcard.h>
#elif defined(__FreeBSD__)
#include <sys/soundcard.h>
#else
#include <machine/soundcard.h> /* JH20010905 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "portaudio.h"
#include "portmixer.h"

typedef struct PxInfo
{
   int index;
   int fd;

   int num_out;
   int outs[SOUND_MIXER_NRDEVICES];
   int num_rec;
   int recs[SOUND_MIXER_NRDEVICES];
} PxInfo;

char PxDevice[20] = "/dev/mixerX";

int PxNumDevices = 0;
int PxDevices[10];

int Px_GetNumMixers( void *pa_stream )
{
   int i;
   int fd;

   PxNumDevices = 0;

   for(i=0; i<11; i++) {
      if (i==0)
         PxDevice[10] = 0;
      else
         PxDevice[10] = '0'+(i-1);
      fd = open(PxDevice, O_RDWR);
      if (fd >= 0) {
         PxDevices[PxNumDevices] = i;
         PxNumDevices++;
         close(fd);
      }
   }

   return PxNumDevices;
}

const char *Px_GetMixerName( void *pa_stream, int index )
{
   if (PxNumDevices <= 0)
      Px_GetNumMixers(pa_stream);

   if (index < 0 || index >= PxNumDevices)
      return NULL;

   if (PxDevices[index]==0)
      PxDevice[10] = 0;
   else
      PxDevice[10] = '0'+(PxDevices[index]-1);
   return PxDevice;
}

PxMixer *Px_OpenMixer( void *pa_stream, int index )
{
   PxInfo *info;
   int devmask, recmask, outmask;
   int i;

   if (PxNumDevices <= 0)
      Px_GetNumMixers(pa_stream);

   if (index < 0 || index >= PxNumDevices)
      return NULL;

   info = (PxInfo *)malloc(sizeof(PxInfo));
   info->index = PxDevice[index];

   if (PxDevices[index]==0)
      PxDevice[10] = 0;
   else
      PxDevice[10] = '0'+(PxDevices[index]-1);
   info->fd = open(PxDevice, O_RDWR);
   if (info->fd < 0)
      goto bad;

   if (ioctl(info->fd, MIXER_READ(SOUND_MIXER_READ_DEVMASK),
             &devmask) == -1)
      goto bad;
   if (ioctl(info->fd, MIXER_READ(SOUND_MIXER_READ_RECMASK),
             &recmask) == -1)
      goto bad;
   outmask = devmask ^ recmask;

   info->num_out = 0;
   info->num_rec = 0;

   for(i=0; i<SOUND_MIXER_NRDEVICES; i++)
      if (recmask & (1<<i))
         info->recs[info->num_rec++] = i;
      else if (devmask & (1<<i))
         info->outs[info->num_out++] = i;

   return (PxMixer *)info;

 bad:
   free(info);
   return NULL;
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

void Px_CloseMixer(PxMixer *mixer)
{
   PxInfo *info = (PxInfo *)mixer;

   close(info->fd);

   free(info);
}

PxVolume GetVolume(int fd, int channel)
{
   int vol;
   int stereo;

   if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereo) == 0)
      stereo = ((stereo & (1 << channel)) != 0);
   else
      stereo = 0;
   
   if (ioctl(fd, MIXER_READ(channel), &vol) == -1)
      return 0.0;

   if (stereo)
      return ((vol & 0xFF)/200.0) + (((vol>>8) & 0xFF)/200.0);
   else
      return (vol & 0xFF)/100.0;
}

/*
 Master (output) volume
*/

PxVolume Px_GetMasterVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return GetVolume(info->fd, SOUND_MIXER_VOLUME);
}

void Px_SetMasterVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   int vol = (int)((volume * 100.0) + 0.5);
   vol = (vol | (vol<<8));
   ioctl(info->fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);
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

   return GetVolume(info->fd, SOUND_MIXER_PCM);
}

void Px_SetPCMOutputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   int vol = (int)((volume * 100.0) + 0.5);
   vol = (vol | (vol<<8));
   ioctl(info->fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol);
}

/*
 All output volumes
*/

int Px_GetNumOutputVolumes( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return info->num_out;
}

const char *Px_GetOutputVolumeName( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   const char *labels[] = SOUND_DEVICE_LABELS;

   return labels[info->outs[i]];
}

PxVolume Px_GetOutputVolume( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;

   return GetVolume(info->fd, info->outs[i]);
}

void Px_SetOutputVolume( PxMixer *mixer, int i, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   int vol = (int)((volume * 100.0) + 0.5);
   vol = (vol | (vol<<8));
   ioctl(info->fd, MIXER_WRITE(info->outs[i]), &vol);
}

/*
 Input sources
*/

int Px_GetNumInputSources( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   
   return info->num_rec;
}

const char *Px_GetInputSourceName( PxMixer *mixer, int i)
{
   PxInfo *info = (PxInfo *)mixer;

   const char *labels[] = SOUND_DEVICE_LABELS;
   return labels[info->recs[i]];
}

int Px_GetCurrentInputSource( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   int recmask;
   int i;

   /* Note that there may be more than one in OSS; we pick
      the first one */

   if (ioctl(info->fd, MIXER_READ(SOUND_MIXER_READ_RECSRC),
             &recmask) == -1)
      return -1; /* none / error */

   for(i=0; i<info->num_rec; i++)
      if (recmask & (1 << (info->recs[i])))
         return i;

   return -1; /* none */
}

void Px_SetCurrentInputSource( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   int newrecsrcmask = (1 << (info->recs[i]));

   ioctl(info->fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC),
         &newrecsrcmask);
}

/*
 Input volume
*/

PxVolume Px_GetInputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   int i;
   
   i = Px_GetCurrentInputSource(mixer);
   if (i < 0)
      return 0.0;

   return GetVolume(info->fd, info->recs[i]);
}

void Px_SetInputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   int vol;
   int i;

   i = Px_GetCurrentInputSource(mixer);
   if (i < 0)
      return;

   vol = (int)((volume * 100.0) + 0.5);
   vol = (vol | (vol<<8));
   ioctl(info->fd, MIXER_WRITE(info->recs[i]), &vol);
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

PxVolume Px_GetPlaythrough( PxMixer *mixer )
{
   return 0.0;
}

void Px_SetPlaythrough( PxMixer *mixer, PxVolume volume )
{
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
