/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#ifndef _AUDIO_PORTAUDIO_H
#define _AUDIO_PORTAUDIO_H

#include "iaxclient_lib.h"

/* normal initialization */
int pa_initialize (struct iaxc_audio_driver *d, int sr);

/* faster initialization which defers initialization of mixers and levels
	until the device is started */
int pa_initialize_deferred (struct iaxc_audio_driver *d, int sr);

#endif
