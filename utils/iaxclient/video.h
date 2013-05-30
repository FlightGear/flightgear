/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Peter Grayson <jpgrayson@gmail.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

struct iaxc_call;

int video_initialize();

int video_destroy();

int video_recv_video(struct iaxc_call * call, int sel_call,
		void * encoded_video, int encoded_video_len,
		unsigned int ts, int format);

#endif
