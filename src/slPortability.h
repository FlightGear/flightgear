
#ifndef __SLPORTABILITY_H__
#define __SLPORTABILITY_H__ 1

/* ------------------------------------------------------------- */
/* OS specific includes and defines ...                          */
/* ------------------------------------------------------------- */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#undef VERSION

#include <stdio.h>
#include <stdlib.h>

#ifndef  WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <windows.h>
#if defined( __CYGWIN__ ) || defined( __CYGWIN32__ )
#  define NEAR /* */
#  define FAR  /* */
#endif
#include <mmsystem.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>

#if defined(__linux__) || defined(__FreeBSD__)
#define SL_USING_OSS_AUDIO 1
#endif

#ifdef SL_USING_OSS_AUDIO
#if defined(__linux__)
#include <linux/soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#else
/*
  Tom thinks this file may be <sys/soundcard.h> under some
  unixen - but that isn't where the OSS manuals say it
  should be.

  If you ever find out the truth, please email me:
     Steve Baker <sjbaker1@airmail.net>
*/
#include <soundcard.h>
#endif
#endif

#ifdef	__OpenBSD__
#include <sys/audioio.h>
#endif

#ifdef WIN32
#define strcasecmp stricmp   /* Yes, Steve really does *HATE* Windoze */
#endif

/* Tom */

#ifdef	sgi
#include <audio.h>
#endif

#endif

