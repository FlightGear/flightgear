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

#include "speex/speex.h"

struct iaxc_speex_settings {
	int decode_enhance;
	float quality;
	int bitrate;
	int vbr;
	int abr; /* abr bitrate */
	int complexity;
};

struct iaxc_audio_codec *codec_audio_speex_new(struct iaxc_speex_settings *settings);
