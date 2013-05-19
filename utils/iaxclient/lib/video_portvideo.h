/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003 HorizonLive.com, (c) 2004, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */
#ifndef _VIDEO_PORTVIDEO_H
#define _VIDEO_PORTVIDEO_H

#include "iaxclient_lib.h"

#ifdef __cplusplus
extern "C" {
#endif


int pv_initialize (struct iaxc_video_driver *d, int w, int h, int framerate);

#ifdef __cplusplus
}
#endif

#endif
