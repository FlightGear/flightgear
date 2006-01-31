
#include <stdio.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#define MAX_DATA	16

int main()
{
   ALCint i,j;
   ALCint data[MAX_DATA];
   ALCdevice *device = NULL;
   ALCcontext *context = NULL;
   const ALCchar *s;
   ALCenum error;
   ALCboolean ret;

   device = alcOpenDevice(NULL);
   if (device == NULL)
   {
      printf("No default audio device available.\n");
      return -1;
   }
   context = alcCreateContext(device, NULL);
   if (context == NULL)
   {
      printf("Could not create a valid context.\n");
      return -2;
   }
   alcMakeContextCurrent(context);

   s = alGetString(AL_VENDOR);
   printf("AL_VENDOR = \"%s\"\n", s);

   s = alGetString(AL_RENDERER);
   printf("AL_RENDERER = \"%s\"\n", s);

   s = alGetString(AL_VERSION);
   printf("AL_VERSION = \"%s\"\n", s);

   s = alGetString(AL_EXTENSIONS);
   printf("AL_EXTENSIONS = \"%s\"\n", s);

   alcGetError(device);

   printf("\n");
   if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
   {
      s = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
      printf("ALC_DEVICE_SPECIFIER = \"%s\"\n", s);
   }

   alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, data);
   printf("ALC_MAJOR_VERSION = %i\n", *data);
   alcGetIntegerv(device, ALC_MINOR_VERSION, 1, data);
   printf("ALC_MINOR_VERSION = %i\n", *data);

   s = alcGetString(device, ALC_EXTENSIONS);
   printf("ALC_EXTENSIONS = \"%s\"\n", s);

   if ((error = alcGetError(device)))
   {
      printf("Error #%i occured\n", error);
      return error;
   }

   s = alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER);
   printf("ALC_DEFAULT_DEVICE_SPECIFIER = \"%s\"\n", s);

   if ((error = alcGetError(device)))
   {
      printf("Error #%i occured\n", error);
      return error;
   }

#if 0
   alcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, 1, &i);
   printf("ALC attributes(%i): ", i);

   alcGetIntegerv(device, ALC_ALL_ATTRIBUTES, i, data);
   for (j=0; j<i; j++)
   {
      printf("%i ", data[j]);
   }
   printf("\n");

   if ((error = alcGetError(device)))
   {
      printf("Error #%i occured\n", error);
      return error;
   }
#endif

   ret = alcCloseDevice(device);

   return ret;
}
