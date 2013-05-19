/*
 * PortMixer
 * Windows WMME Implementation
 *
 * Copyright (c) 2002
 *
 * Written by Dominic Mazzoni and Augustus Saunders
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

#include <windows.h>

#include "portaudio.h"
#include "portmixer.h"

#if defined(PaStream)
#define PA_V18
#else
#define PA_V19
#endif

#if defined(PA_V18)
#include "pa_host.h"

typedef struct PaWMMEStreamData
{
    /* Input -------------- */
    HWAVEIN            hWaveIn;
    WAVEHDR           *inputBuffers;
    int                currentInputBuffer;
    int                bytesPerHostInputBuffer;
    int                bytesPerUserInputBuffer;    /* native buffer size in bytes */
    /* Output -------------- */
    HWAVEOUT           hWaveOut;
} PaWMMEStreamData;
#endif

#if defined(PA_V19)
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_stream.h"

typedef struct
{
    HANDLE bufferEvent;
    void *waveHandles;
    unsigned int deviceCount;
    /* unsigned int channelCount; */
    WAVEHDR **waveHeaders;                  /* waveHeaders[device][buffer] */
    unsigned int bufferCount;
    unsigned int currentBufferIndex;
    unsigned int framesPerBuffer;
    unsigned int framesUsedInCurrentBuffer;
}PaWinMmeSingleDirectionHandlesAndBuffers;

/* PaWinMmeStream - a stream data structure specifically for this implementation */
/* note that struct PaWinMmeStream is typedeffed to PaWinMmeStream above. */
struct PaWinMmeStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    int primeStreamUsingCallback;

    PaWinMmeSingleDirectionHandlesAndBuffers input;
    PaWinMmeSingleDirectionHandlesAndBuffers output;

    /* Processing thread management -------------- */
    HANDLE abortEvent;
    HANDLE processingThread;
    DWORD processingThreadId;

    char throttleProcessingThreadOnOverload; /* 0 -> don't throtte, non-0 -> throttle */
    int processingThreadPriority;
    int highThreadPriority;
    int throttledThreadPriority;
    unsigned long throttledSleepMsecs;

    int isStopped;
    volatile int isActive;
    volatile int stopProcessing; /* stop thread once existing buffers have been returned */
    volatile int abortProcessing; /* stop thread immediately */

    DWORD allBuffersDurationMs; /* used to calculate timeouts */
};
#endif

typedef struct PxSrcInfo
{
   char  name[256];
   DWORD lineID;
   DWORD controlID;
} PxSrcInfo;

typedef struct PxInfo
{
   HMIXEROBJ   hInputMixer;
   HMIXEROBJ   hOutputMixer;
   int         numInputs;
   PxSrcInfo   src[32];
   DWORD       muxID;
   DWORD       speakerID;
   DWORD       waveID;
} PxInfo;

int Px_GetNumMixers( void *pa_stream )
{
   return 1;
}

const char *Px_GetMixerName( void *pa_stream, int index )
{
   return "Mixer";
}

PxMixer *Px_OpenMixer( void *pa_stream, int index )
{
#if defined(PA_V18)
   internalPortAudioStream     *past;
   PaWMMEStreamData            *wmmeStreamData;
#endif

#if defined(PA_V19)
   struct PaWinMmeStream       *past;
#endif

   HWAVEIN                      hWaveIn;
   HWAVEOUT                     hWaveOut;
   PxInfo                      *mixer;
   MMRESULT                     result;
   MIXERLINE                    line;
   MIXERLINECONTROLS            controls;
   MIXERCONTROL                 control;
   MIXERCONTROLDETAILS          details;
   MIXERCONTROLDETAILS_LISTTEXT mixList[32];
   int                          j;

   if (!pa_stream)
      return NULL;

   mixer = (PxInfo *)malloc(sizeof(PxInfo));
   mixer->hInputMixer = NULL;
   mixer->hOutputMixer = NULL;

#if defined(PA_V18)
   past = (internalPortAudioStream *) pa_stream;
   wmmeStreamData = (PaWMMEStreamData *) past->past_DeviceData;

   hWaveIn = wmmeStreamData->hWaveIn;
   hWaveOut = wmmeStreamData->hWaveOut;
#endif

#if defined(PA_V19)
   past = (struct PaWinMmeStream *) pa_stream;

   hWaveIn = 0;
   if (past->input.waveHandles) {
      hWaveIn = ((HWAVEIN *)past->input.waveHandles)[0];
   }

   hWaveOut = 0;
   if (past->output.waveHandles) {
      hWaveOut = ((HWAVEOUT *)past->output.waveHandles)[0];
   }
#endif

   if (hWaveIn) {
      result = mixerOpen((HMIXER *)&mixer->hInputMixer, (UINT)hWaveIn, 0, 0, MIXER_OBJECTF_HWAVEIN);
      if (result != MMSYSERR_NOERROR) {
         free(mixer);
         return NULL;
      }
   }

   if (hWaveOut) {
      result = mixerOpen((HMIXER *)&mixer->hOutputMixer, (UINT)hWaveOut, 0, 0, MIXER_OBJECTF_HWAVEOUT);
      if (result != MMSYSERR_NOERROR) {
         free(mixer);
         return NULL;
      }
   }

   mixer->numInputs = 0;
   mixer->muxID = 0;

	/* ??? win32 default for wave control seems to be 0 ??? */
	mixer->waveID = 0 ; 

   /*
    * Find the input source selector (mux or mixer) and
    * get the names and IDs of all of the input sources
    */

   if (mixer->hInputMixer) {
      line.cbStruct = sizeof(MIXERLINE);
      line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
      result = mixerGetLineInfo(mixer->hInputMixer,
                                &line,
                                MIXER_GETLINEINFOF_COMPONENTTYPE);
      if (result == MMSYSERR_NOERROR) {

         controls.cbStruct = sizeof(MIXERLINECONTROLS);
         controls.dwLineID = line.dwLineID;
         controls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
         controls.cbmxctrl = sizeof(MIXERCONTROL);
         controls.pamxctrl = &control;

         control.cbStruct = sizeof(MIXERCONTROL);

         result = mixerGetLineControls(mixer->hInputMixer,
                                       &controls,
                                       MIXER_GETLINECONTROLSF_ONEBYTYPE);

         if (result != MMSYSERR_NOERROR) {
            controls.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;
            result = mixerGetLineControls(mixer->hInputMixer,
                                          &controls,
                                          MIXER_GETLINECONTROLSF_ONEBYTYPE);
         }

         if (result == MMSYSERR_NOERROR) {
            mixer->numInputs = control.cMultipleItems;
            mixer->muxID = control.dwControlID;

            details.cbStruct = sizeof(MIXERCONTROLDETAILS);
            details.dwControlID = mixer->muxID;
            details.cChannels = 1;
            details.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
            details.paDetails = (LPMIXERCONTROLDETAILS_LISTTEXT)&mixList[0];
            details.cMultipleItems = mixer->numInputs;

            result = mixerGetControlDetails(mixer->hInputMixer,
                                            (LPMIXERCONTROLDETAILS)&details,
                                            MIXER_GETCONTROLDETAILSF_LISTTEXT);

            if (result != MMSYSERR_NOERROR)
               mixer->numInputs = 0;

            for(j=0; j<mixer->numInputs; j++) {
               mixer->src[j].lineID = mixList[j].dwParam1;
               strcpy(mixer->src[j].name, mixList[j].szName);
            }
         }

         /*
          * We've found the names - now we need to find the volume
          * controls for each one
          */

         for(j=0; j<mixer->numInputs; j++) {
            controls.cbStruct = sizeof(MIXERLINECONTROLS);
            controls.dwLineID = mixer->src[j].lineID;
            controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
            controls.cbmxctrl = sizeof(MIXERCONTROL);
            controls.pamxctrl = &control;

            control.cbStruct = sizeof(MIXERCONTROL);

            result = mixerGetLineControls(mixer->hInputMixer,
                                          &controls,
                                          MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (result == MMSYSERR_NOERROR)
               mixer->src[j].controlID = control.dwControlID;
            else
               mixer->src[j].controlID = 0;
         }
      }
   }

   /*
    * Find the ID of the output speaker volume control
    */

   mixer->speakerID = 0;
   
   if (mixer->hOutputMixer) {
      line.cbStruct = sizeof(MIXERLINE);
      line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
      result = mixerGetLineInfo(mixer->hOutputMixer,
                                &line,
                                MIXER_GETLINEINFOF_COMPONENTTYPE);
      if (result == MMSYSERR_NOERROR) {
         controls.cbStruct = sizeof(MIXERLINECONTROLS);
         controls.dwLineID = line.dwLineID;
         controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
         controls.cbmxctrl = sizeof(MIXERCONTROL);
         controls.pamxctrl = &control;

         control.cbStruct = sizeof(MIXERCONTROL);

         result = mixerGetLineControls(mixer->hOutputMixer,
                                       &controls,
                                       MIXER_GETLINECONTROLSF_ONEBYTYPE);

         if (result == MMSYSERR_NOERROR)
            mixer->speakerID = control.dwControlID;
      }
   }

   return (PxMixer *)mixer;
}

void VolumeFunction(HMIXEROBJ hMixer, DWORD controlID, PxVolume *volume)
{
   MIXERCONTROLDETAILS details;
   MMRESULT result;
   MIXERCONTROLDETAILS_UNSIGNED value;

   memset(&value, 0, sizeof(MIXERCONTROLDETAILS_UNSIGNED));

   details.cbStruct = sizeof(MIXERCONTROLDETAILS);
   details.dwControlID = controlID;
   details.cChannels = 1; /* all channels */
   details.cMultipleItems = 0;
   details.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
   details.paDetails = &value;

   result = mixerGetControlDetails(hMixer, &details,
                                   MIXER_GETCONTROLDETAILSF_VALUE);

   if (*volume < 0.0) {
      *volume = (PxVolume)(value.dwValue / 65535.0);
   }
   else {
      if (result != MMSYSERR_NOERROR)
         return;
      value.dwValue = (unsigned short)(*volume * 65535.0);
      mixerSetControlDetails(hMixer, &details,
                             MIXER_GETCONTROLDETAILSF_VALUE);
   }
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

void Px_CloseMixer(PxMixer *mixer)
{
   PxInfo *info = (PxInfo *)mixer;

   if (info->hInputMixer)
      mixerClose((HMIXER)info->hInputMixer);
   if (info->hOutputMixer)
      mixerClose((HMIXER)info->hOutputMixer);
   free( mixer );
}

/*
 Master (output) volume
*/

PxVolume Px_GetMasterVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   PxVolume vol;

   vol = -1.0;
   VolumeFunction(info->hOutputMixer, info->speakerID, &vol);
   return vol;
}

void Px_SetMasterVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   VolumeFunction(info->hOutputMixer, info->speakerID, &volume);
}

/*
 PCM output volume
*/

int Px_SupportsPCMOutputVolume( PxMixer* mixer ) 
{
	PxInfo* info = ( PxInfo* )( mixer ) ;
	return ( info->waveID == -1 ) ? 0 : 1 ;
}

PxVolume Px_GetPCMOutputVolume( PxMixer *mixer )
{
  MMRESULT result;
  DWORD vol = 0;
  unsigned short mono_vol = 0;
  PxInfo *info = (PxInfo *)mixer;

	/* invalid waveID, return zero */
	if ( info->waveID == -1 )
		return 0.0 ;

	/* get the wave output volume */
	result = waveOutGetVolume( (HWAVEOUT)( info->waveID ), &vol);

	/* on failure, mark waveID as invalid and return zero */
	if ( result != MMSYSERR_NOERROR )
	{
		info->waveID = -1 ;
		return 0.0 ;
	}

  mono_vol = (unsigned short)vol;
  return (PxVolume)mono_vol/65535.0;
}

void Px_SetPCMOutputVolume( PxMixer *mixer, PxVolume volume )
{
	MMRESULT result;
	PxInfo *info = (PxInfo *)mixer;

	/* invalid waveID */
	if ( info->waveID == -1 )
		return ;

	/* set the wave output volume */
	result = waveOutSetVolume( (HWAVEOUT)( info->waveID ), MAKELONG(volume*0xFFFF, volume*0xFFFF));

	/* on failure, mark waveID as invalid  */
	if ( result != MMSYSERR_NOERROR )
	{
		info->waveID = -1 ;
	}

	return ;
}

/*
 All output volumes
*/

int Px_GetNumOutputVolumes( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;

   return 2;
}

const char *Px_GetOutputVolumeName( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   
   if (i==1)
      return "Wave Out";
   else
      return "Master Volume";
}

PxVolume Px_GetOutputVolume( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;

   if (i==1)
      return Px_GetPCMOutputVolume(mixer);
   else
      return Px_GetMasterVolume(mixer);
}

void Px_SetOutputVolume( PxMixer *mixer, int i, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;

   if (i==1)
      Px_SetPCMOutputVolume(mixer, volume);
   else
      Px_SetMasterVolume(mixer, volume);
}

/*
 Input sources
*/

int Px_GetNumInputSources( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   
   return info->numInputs;
}

const char *Px_GetInputSourceName( PxMixer *mixer, int i)
{
   PxInfo *info = (PxInfo *)mixer;
   
   return info->src[i].name;
}

int Px_GetCurrentInputSource( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   MIXERCONTROLDETAILS details;
   MIXERCONTROLDETAILS_BOOLEAN flags[32];
   MMRESULT result;
   int i;

   details.cbStruct = sizeof(MIXERCONTROLDETAILS);
   details.dwControlID = info->muxID;
   details.cMultipleItems = info->numInputs;
   details.cChannels = 1;   
   details.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
   details.paDetails = (LPMIXERCONTROLDETAILS_BOOLEAN)&flags[0];

   result = mixerGetControlDetails(info->hInputMixer,
                                   (LPMIXERCONTROLDETAILS)&details,
                                   MIXER_GETCONTROLDETAILSF_VALUE);

   if (result == MMSYSERR_NOERROR) {
      for(i=0; i<info->numInputs; i++)
         if (flags[i].fValue)
            return i;
   }

   return 0;
}

void Px_SetCurrentInputSource( PxMixer *mixer, int i )
{
   PxInfo *info = (PxInfo *)mixer;
   MIXERCONTROLDETAILS details;
   MIXERCONTROLDETAILS_BOOLEAN flags[32];
   MMRESULT result;
   int j;

   details.cbStruct = sizeof(MIXERCONTROLDETAILS);
   details.dwControlID = info->muxID;
   details.cMultipleItems = info->numInputs;
   details.cChannels = 1;   
   details.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
   details.paDetails = (LPMIXERCONTROLDETAILS_BOOLEAN)&flags[0];

   for(j=0; j<info->numInputs; j++)
      flags[j].fValue = (i == j);

   result = mixerSetControlDetails(info->hInputMixer,
                                   (LPMIXERCONTROLDETAILS)&details,
                                   MIXER_SETCONTROLDETAILSF_VALUE);
}

/*
 Input volume
*/

PxVolume Px_GetInputVolume( PxMixer *mixer )
{
   PxInfo *info = (PxInfo *)mixer;
   PxVolume vol;
   int src = Px_GetCurrentInputSource(mixer);

   vol = -1.0;
   VolumeFunction(info->hInputMixer, info->src[src].controlID, &vol);
   return vol;
}

void Px_SetInputVolume( PxMixer *mixer, PxVolume volume )
{
   PxInfo *info = (PxInfo *)mixer;
   int src = Px_GetCurrentInputSource(mixer);

   VolumeFunction(info->hInputMixer, info->src[src].controlID, &volume);
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





int Px_SetMicrophoneBoost( PxMixer* mixer, int enable )
{
	MMRESULT mmr = MMSYSERR_ERROR ;
	
	// cast void pointer
	PxInfo* info = ( PxInfo* )( mixer ) ;

	if ( info == NULL ) 
		return MMSYSERR_ERROR ;
		
	//
	// get line info
	//
	
	MIXERLINE mixerLine ;
    mixerLine.cbStruct = sizeof( MIXERLINE ) ;
    mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE ;
	
	mmr = mixerGetLineInfo(
		( HMIXEROBJ )( info->hInputMixer ),
		&mixerLine,
		MIXER_GETLINEINFOF_COMPONENTTYPE
	) ;

	if ( mmr != MMSYSERR_NOERROR )
		return mmr ;

	//
	// get all controls
	//

	LPMIXERCONTROL mixerControl = (LPMIXERCONTROL)malloc( sizeof( MIXERCONTROL ) * mixerLine.cControls ) ;
	
	MIXERLINECONTROLS mixerLineControls ;
	mixerLineControls.cbStruct = sizeof( MIXERLINECONTROLS ) ;
	mixerLineControls.dwLineID = mixerLine.dwLineID ;
	mixerLineControls.cControls = mixerLine.cControls ;
	mixerLineControls.cbmxctrl = sizeof( MIXERCONTROL ) ;
	mixerLineControls.pamxctrl = ( LPMIXERCONTROL )( mixerControl ) ;
	
	mmr = mixerGetLineControls(
		( HMIXEROBJ )( info->hInputMixer ), 
		&mixerLineControls,
		MIXER_GETLINECONTROLSF_ALL
	) ;
	
	if ( mmr != MMSYSERR_NOERROR )
		return mmr ;

	//
	// find boost control
	//

	DWORD boost_id = -1 ;
	int x = 0 ;
	
	for ( ; x < mixerLineControls.cControls ; ++x )
	{
		// check control type
		if ( mixerControl[x].dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF )
		{
			// normalize control name
			char* name = _strupr( mixerControl[x].szName ) ;

			// check for 'mic' and 'boost'
			if (
				( strstr( name, "MIC" ) != NULL )
				&& ( strstr( name, "BOOST" ) != NULL )
			)
			{
				boost_id = mixerControl[x].dwControlID ;
				break ;
			}
		}
	}

	if ( boost_id == -1 )
		return MMSYSERR_ERROR ;

	//
	// get control details
	//
	
	MIXERCONTROLDETAILS_BOOLEAN value ;

	MIXERCONTROLDETAILS mixerControlDetails ;
	mixerControlDetails.cbStruct = sizeof( MIXERCONTROLDETAILS ) ;
	mixerControlDetails.dwControlID = boost_id ;
	mixerControlDetails.cChannels = 1 ;
	mixerControlDetails.cMultipleItems = 0 ;
	mixerControlDetails.cbDetails = sizeof( MIXERCONTROLDETAILS_BOOLEAN ) ;
	mixerControlDetails.paDetails = &value ;

	mmr = mixerGetControlDetails( 
		( HMIXEROBJ )( info->hInputMixer ),
		&mixerControlDetails,
		MIXER_GETCONTROLDETAILSF_VALUE
	) ;

	if ( mmr != MMSYSERR_NOERROR )
		return mmr ;

	//
	// update value
	//

	value.fValue = ( enable == 0 ) ? 0L : 1L ;

	//
	// set control details
	//
	
	mmr = mixerSetControlDetails( 
		( HMIXEROBJ )( info->hInputMixer ),
		&mixerControlDetails,
		MIXER_SETCONTROLDETAILSF_VALUE
	) ;

	if ( mmr != MMSYSERR_NOERROR )
		return mmr ;
	
	return mmr ;
}

int Px_GetMicrophoneBoost( PxMixer* mixer )
{
	MMRESULT mmr = MMSYSERR_ERROR ;
	
	// cast void pointer
	PxInfo* info = ( PxInfo* )( mixer ) ;

	if ( info == NULL ) 
		return -1 ;
		
	//
	// get line info
	//
	
	MIXERLINE mixerLine ;
    mixerLine.cbStruct = sizeof( MIXERLINE ) ;
    mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE ;
	
	mmr = mixerGetLineInfo(
		( HMIXEROBJ )( info->hInputMixer ),
		&mixerLine,
		MIXER_GETLINEINFOF_COMPONENTTYPE
	) ;

	if ( mmr != MMSYSERR_NOERROR )
		return -1 ;

	//
	// get all controls
	//

	LPMIXERCONTROL mixerControl = (LPMIXERCONTROL)malloc( sizeof( MIXERCONTROL ) * mixerLine.cControls ) ;
	
	MIXERLINECONTROLS mixerLineControls ;
	mixerLineControls.cbStruct = sizeof( MIXERLINECONTROLS ) ;
	mixerLineControls.dwLineID = mixerLine.dwLineID ;
	mixerLineControls.cControls = mixerLine.cControls ;
	mixerLineControls.cbmxctrl = sizeof( MIXERCONTROL ) ;
	mixerLineControls.pamxctrl = ( LPMIXERCONTROL )( mixerControl ) ;
	
	mmr = mixerGetLineControls(
		( HMIXEROBJ )( info->hInputMixer ), 
		&mixerLineControls,
		MIXER_GETLINECONTROLSF_ALL
	) ;
	
	if ( mmr != MMSYSERR_NOERROR )
		return -1 ;

	//
	// find boost control
	//

	DWORD boost_id = -1 ;
	int x = 0 ;
	
	for ( ; x < mixerLineControls.cControls ; ++x )
	{
		// check control type
		if ( mixerControl[x].dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF )
		{
			// normalize control name
			char* name = _strupr( mixerControl[x].szName ) ;

			// check for 'mic' and 'boost'
			if (
				( strstr( name, "MIC" ) != NULL )
				&& ( strstr( name, "BOOST" ) != NULL )
			)
			{
				boost_id = mixerControl[x].dwControlID ;
				break ;
			}
		}
	}

	if ( boost_id == -1 )
		return -1 ;

	//
	// get control details
	//
	
	MIXERCONTROLDETAILS_BOOLEAN value ;

	MIXERCONTROLDETAILS mixerControlDetails ;
	mixerControlDetails.cbStruct = sizeof( MIXERCONTROLDETAILS ) ;
	mixerControlDetails.dwControlID = boost_id ;
	mixerControlDetails.cChannels = 1 ;
	mixerControlDetails.cMultipleItems = 0 ;
	mixerControlDetails.cbDetails = sizeof( MIXERCONTROLDETAILS_BOOLEAN ) ;
	mixerControlDetails.paDetails = &value ;

	mmr = mixerGetControlDetails( 
		( HMIXEROBJ )( info->hInputMixer ),
		&mixerControlDetails,
		MIXER_GETCONTROLDETAILSF_VALUE
	) ;

	if ( mmr != MMSYSERR_NOERROR )
		return -1 ;
	
	return ( int )( value.fValue ) ;
}

int Px_SetCurrentInputSourceByName( PxMixer* mixer, const char* name ) 
{
	// cast void pointer
	PxInfo* info = ( PxInfo* )( mixer ) ;

	// make sure we have a mixer
	if ( info == NULL ) 
		return MMSYSERR_ERROR ;

	// make sure we have a search name
	if ( name == NULL )
		return MMSYSERR_ERROR ;

	//
	// set input source
	//

	int x = 0 ;
	for ( ; x < info->numInputs ; ++x )
	{
		// compare passed name with control name
		if ( strncasecmp( info->src[x].name, name, strlen( name ) ) == 0 )
		{
			// set input source
			Px_SetCurrentInputSource( mixer, x ) ;
			
			// make sure set'ing worked
			if ( Px_GetCurrentInputSource( mixer ) == x )
				return MMSYSERR_NOERROR ;
			else
				return MMSYSERR_ERROR ;
		}
	}

	return MMSYSERR_ERROR ;
}

