/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alcinfo.x
 *
 * alcinfo display info about a ALC extension and OpenAL renderer
 *
 * This file is in the Public Domain and comes with no warranty.
 * Erik Hofman <erik@ehofman.com>
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef __APPLE__
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
# include <AL/alext.h>
#endif

#ifndef AL_VERSION_1_1
# ifdef __APPLE__
#  include <OpenAL/altypes.h>
#  include <OpenAL/alctypes.h>
#else
#  include <AL/altypes.h>
#  include <AL/alctypes.h>
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>

using std::string;

#ifndef ALC_ALL_DEVICES_SPECIFIER
# define ALC_ALL_DEVICES_SPECIFIER      0x1013
#endif

#define MAX_DATA	16
static const int indentation = 4;
static const int maxmimumWidth = 79;

void printExtensions (const char *, char, const char *);
void displayDevices(const char *, const char *);
char *getDeviceName(int, char **);
void testForError(void *, const string&);
void testForALCError(ALCdevice *);

int main(int argc, char **argv)
{
   ALCint data[MAX_DATA];
   ALCdevice *device = NULL;
   ALCcontext *context = NULL;
   ALenum error;
   char *s;

   if (alcIsExtensionPresent(NULL, "ALC_enumeration_EXT") == AL_TRUE)
   {
      if (alcIsExtensionPresent(NULL, "ALC_enumerate_all_EXT") == AL_FALSE)
         s = (char *)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
      else
         s = (char *)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
      displayDevices("output", s);

      s = (char *)alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
      displayDevices("input", s);
   }

   s = getDeviceName(argc, argv);
   device = alcOpenDevice(s);
   testForError(device, "Audio device not available.");

   context = alcCreateContext(device, NULL);
   testForError(context, "Unable to create a valid context.");

   alcMakeContextCurrent(context);
   testForALCError(device);

   s = (char *)alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER);
   printf("default output device: %s\n", s);
   testForALCError(device);

   error = alcIsExtensionPresent(device, "ALC_EXT_capture");
   if (error)
   {
      s = (char *)alcGetString(device, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
      printf("default input device:  %s\n", s);
      testForALCError(device);
   }
   printf("capture support: %s\n", (error) ? "yes" : "no");

   alcGetIntegerv(device, ALC_FREQUENCY, 1, data);
   printf("mixer frequency: %u hz\n", data[0]);
   testForALCError(device);

   alcGetIntegerv(device, ALC_REFRESH, 1, data+1);
   printf("refresh rate : %u hz\n", data[0]/data[1]);
   testForALCError(device);

   data[0] = 0;
   alcGetIntegerv(device, ALC_MONO_SOURCES, 1, data);
   error = alcGetError(device);
   if (error == AL_NONE) {
      printf("supported sources; mono: %u, ", data[0]);

      data[0] = 0;
      alcGetIntegerv(device, ALC_STEREO_SOURCES, 1, data);
      printf("stereo: %u\n", data[0]);
      testForALCError(device);
   }

   printf("ALC version: ");
   alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, data);
   printf("%i.", *data);
   alcGetIntegerv(device, ALC_MINOR_VERSION, 1, data);
   printf("%i\n", *data);
   testForALCError(device);

   s = (char *)alcGetString(device, ALC_EXTENSIONS);
   printExtensions ("ALC extensions", ' ', s);
   testForALCError(device);

   s = (char *)alGetString(AL_VENDOR);
   error = alGetError();
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("Error #%x: %s\n", error, alGetString(error));
   else
      printf("OpenAL vendor string: %s\n", s);

   s = (char *)alGetString(AL_RENDERER);
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("Error #%x: %s\n", error, alGetString(error));
   else
      printf("OpenAL renderer string: %s\n", s);

   s = (char *)alGetString(AL_VERSION);
   if ((error = alGetError()) != AL_NO_ERROR)
      printf("Error #%x: %s\n", error, alGetString(error));
   else if (!s)
      printf("Quering AL_VERSION returned NULL pointer!\n");
   else
      printf("OpenAL version string: %s\n", s);

   s = (char *)alGetString(AL_EXTENSIONS);
   printExtensions ("OpenAL extensions", ' ', s);
   testForALCError(device);

/* alut testing mechanism */
   context = alcGetCurrentContext();
   if (context == NULL)
   {
      printf("Error: no current context\n");
   }
   else
   {
      if (alGetError () != AL_NO_ERROR)
      {
         printf("Alert: AL error on entry\n");
      }
      else
      {
         if (alcGetError (alcGetContextsDevice (context)) != ALC_NO_ERROR)
         {
            printf("Alert: ALC error on entry\n");
         }
      }
   }
/* end of alut test */
 
   
   if (alcMakeContextCurrent(NULL) == 0)
      printf("alcMakeContextCurrent failed.\n");

   device = alcGetContextsDevice(context);
   alcDestroyContext(context);
   testForALCError(device);

   if (alcCloseDevice(device) == 0)
      printf("alcCloseDevice failed.\n");

   return 0;
}

/* -------------------------------------------------------------------------- */

void
printChar (int c, int *width)
{
  putchar (c);
  *width = (c == '\n') ? 0 : (*width + 1);
}

void
indent (int *width)
{
  int i;
  for (i = 0; i < indentation; i++)
    {
      printChar (' ', width);
    }
}

void
printExtensions (const char *header, char separator, const char *extensions)
{
  int width = 0, start = 0, end = 0;

  printf ("%s:\n", header);
  if (extensions == NULL || extensions[0] == '\0')
    {
      return;
    }

  indent (&width);
  while (1)
    {
      if (extensions[end] == separator || extensions[end] == '\0')
        {
          if (width + end - start + 2 > maxmimumWidth)
            {
              printChar ('\n', &width);
              indent (&width);
            }
          while (start < end)
            {
              printChar (extensions[start], &width);
              start++;
            }
          if (extensions[end] == '\0')
            {
              break;
            }
          start++;
          end++;
          if (extensions[end] == '\0')
            {
              break;
            }
          printChar (',', &width);
          printChar (' ', &width);
        }
      end++;
    }
  printChar ('\n', &width);
}

char *
getCommandLineOption(int argc, char **argv, const string& option)
{
   int slen = option.size();
   char *rv = 0;
   int i;

   for (i=0; i<argc; i++)
   {
      if (strncmp(argv[i], option.c_str(), slen) == 0)
      {
         i++;
         if (i<argc) rv = argv[i];
      }
   }

   return rv;
}

char *
getDeviceName(int argc, char **argv)
{
   static char devname[255];
   int len = 255;
   char *s;

   s = getCommandLineOption(argc, argv, "-d");
   if (s)
   {
      strncpy((char *)&devname, s, len);
      len -= strlen(s);

      s = getCommandLineOption(argc, argv, "-r");
      if (s)
      {
         strncat((char *)&devname, " on ", len);
         len -= 4;

         strncat((char *)&devname, s, len);
      }
      s = (char *)&devname;
   }

   return s;
}

void
displayDevices(const char *type, const char *list)
{
   ALCchar *ptr, *nptr;

   ptr = (ALCchar *)list;
   printf("list of all available %s devices:\n", type);
   if (!list)
   {
      printf("none\n");
   }
   else
   {
      nptr = ptr;
      while (*(nptr += strlen(ptr)+1) != 0)
      {
         printf("  %s\n", ptr);
         ptr = nptr;
      }
      printf("  %s\n", ptr);
   }
} 

void
testForError(void *p, const string& s)
{
   if (p == NULL)
   {
      printf("\nError: %s\n\n", s.c_str());
      exit(-1);
   }
}

void
testForALCError(ALCdevice *device)
{
   ALenum error;
   error = alcGetError(device);
   if (error != ALC_NO_ERROR)
      printf("\nALC Error %x occurred: %s\n", error, alcGetString(device, error));
}
