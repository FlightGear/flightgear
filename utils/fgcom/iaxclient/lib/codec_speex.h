/*
 * iaxclient: a portable telephony toolkit
 *
 * Copyright (C) 2003-2004, Horizon Wimba, Inc.
 *
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

#include "speex/speex.h"

#define IAXC_SPEEX_TERMINATION_NORMAL 0  /* normal asterisk-compatible mode:  send terminator at end of each "chunk" */
#define IAXC_SPEEX_TERMINATION_EVERY  1  /* include a terminator after each 20ms frame */
#define IAXC_SPEEX_TERMINATION_NONE   2  /* include no terminators at all, encode/decode one frame at a time */

struct iaxc_speex_settings {
	int decode_enhance;
	float quality;
	int bitrate;
	int vbr;
	int abr; /* abr bitrate */
	int complexity;
	const SpeexMode *mode;
	int termination_mode;
};

struct iaxc_audio_codec *iaxc_audio_codec_speex_new(struct iaxc_speex_settings *settings);
