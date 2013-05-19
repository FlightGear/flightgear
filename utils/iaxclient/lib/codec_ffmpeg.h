/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Peter Grayson <jpgrayson@gmail.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 *
 * A video codec using the ffmpeg library.
 */

struct iaxc_video_codec *codec_video_ffmpeg_new(int format, int w, int h, int framerate, int bitrate, int fragsize);

int codec_video_ffmpeg_check_codec(int format);
