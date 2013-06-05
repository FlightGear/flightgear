#include <stdio.h>

#include "portmixer.h"
#include "portaudio.h"

static int DummyCallbackFunc(const void *inputBuffer, 
			     void *outputBuffer,
                             unsigned long framesPerBuffer,
			     const PaStreamCallbackTimeInfo* timeInfo,
			     PaStreamCallbackFlags statusFlags,
			     void *userData )
{
   return 0;
}
#define NUM_CHANNELS    (2)
#define PA_SAMPLE_TYPE  paFloat32
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

int main(int argc, char **argv)
{
   int num_mixers;
   int i;
   PaError          error;
   PaStream*       stream;
   PaStreamParameters inputParameters;
   PaStreamParameters outputParameters;

   error = Pa_Initialize();
   if ( error != paNoError ) {
      printf("PortAudio error %d: %s\n", error, Pa_GetErrorText(error));
      return -1;
   }
   inputParameters.device = Pa_GetDefaultInputDevice();
   inputParameters.channelCount = NUM_CHANNELS;
   inputParameters.sampleFormat = PA_SAMPLE_TYPE;
   inputParameters.suggestedLatency = 
       Pa_GetDeviceInfo(inputParameters.device )->defaultLowInputLatency;
   inputParameters.hostApiSpecificStreamInfo = NULL;

   outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
   outputParameters.channelCount = 2;       /* stereo output */
   outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
   outputParameters.suggestedLatency = 
       Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
   outputParameters.hostApiSpecificStreamInfo = NULL;



   error = Pa_OpenStream(&stream, &inputParameters, &outputParameters, 
			 SAMPLE_RATE,FRAMES_PER_BUFFER,
                         paClipOff | paDitherOff,
                         DummyCallbackFunc, NULL);

   if (error) {
      printf("PortAudio error %d: %s\n", error, Pa_GetErrorText(error));
      return -1;
   }
   
   num_mixers = Px_GetNumMixers(stream);
   printf("Number of mixers: %d\n", num_mixers);
   for(i=0; i<num_mixers; i++) {
      PxMixer *mixer;
      int num;
      int j;

      printf("Mixer %d: %s\n", i, Px_GetMixerName(stream, i));
      mixer = Px_OpenMixer(stream, i);
      if (!mixer) {
         printf("  Could not open mixer!\n");
         continue;
      }
      
      printf("  Master volume: %.2f\n", Px_GetMasterVolume(mixer));
      printf("  PCM output volume: %.2f\n", Px_GetPCMOutputVolume(mixer));

      num = Px_GetNumOutputVolumes(mixer);
      printf("  Num outputs: %d\n", num);
      for(j=0; j<num; j++) {
         printf("    Output %d (%s): %.2f\n",
                j,
                Px_GetOutputVolumeName(mixer, j),
                Px_GetOutputVolume(mixer, j));
      }

      num = Px_GetNumInputSources(mixer);
      printf("  Num input sources: %d\n", num);
      for(j=0; j<num; j++) {
         printf("    Input %d (%s) %s\n",
                j,
                Px_GetInputSourceName(mixer, j),
                (Px_GetCurrentInputSource(mixer)==j?
                 "SELECTED": ""));
      }
      printf("  Input volume: %.2f\n", Px_GetInputVolume(mixer));

      Px_CloseMixer(mixer);
   }

   Pa_CloseStream(stream);

   return 0;
}
