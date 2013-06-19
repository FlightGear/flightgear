/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2005-2006, Horizon Wimba, inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Mihai Balea <mihai AT hates DOT ms>
 * Peter Grayson <jpgrayson@gmail.com>
 * Erik Bunce <kde@bunce.us>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#include <assert.h>
#include <stdlib.h>

#include <vidcap/vidcap.h>
#include <vidcap/converters.h>

#include "video.h"
#include "slice.h"
#include "iaxclient_lib.h"
#include "iax-client.h"
#ifdef USE_FFMPEG
#include "codec_ffmpeg.h"
#endif
#ifdef USE_THEORA
#include "codec_theora.h"
#endif

#if defined(WIN32)
#define strdup _strdup
#endif

struct video_info
{
	vidcap_state * vc;
	vidcap_sapi * sapi;
	vidcap_src * src;
	struct vidcap_sapi_info sapi_info;
	struct vidcap_fmt_info fmt_info;

	/* these are the requested (post-scaling) dimensions */
	int width;
	int height;

	MUTEX camera_lock;
	int capturing;

	char * converted_i420_buf;
	int converted_i420_buf_size;
	int (*convert_to_i420)(int, int, const char *, char *);

	char * converted_rgb_buf;
	int converted_rgb_buf_size;
	int (*convert_to_rgb32)(int, int, const char *, char *);

	char * scaled_buf;
	int scaled_buf_size;
	void (*scale_image)(const unsigned char *, int, int,
			unsigned char *, int, int);

	int prefs;

	struct slicer_context * sc;

	/* these two struct arrays are correlated by index */
	struct vidcap_src_info * vc_src_info;
	struct iaxc_video_device * devices;
	MUTEX dev_list_lock;

	int device_count;
	int selected_device_id;
	int next_id;
};

struct video_format_info
{
	int width;
	int height;
	int framerate;
	int bitrate;
	int fragsize;

	/* Note that here format really means codec (thoera, h264, etc) */
	int format_preferred;
	int format_allowed;
};

static struct video_info vinfo;

/* TODO: This vfinfo instance is ... funny. The current semantic of
 * iaxc_video_format_set() requires it to be called _prior_ to
 * iaxc_initialize() which of course is where video initialize is called.
 * This means that no code in this video.c module is called prior to
 * iaxc_video_format_set(). This is silly, wrong, and bad.
 *
 * What would be better would be if iaxc_video_format_set() was called
 * by clients _after_ iaxc_initialize(). The TODO here is to do the
 * analysis and restructure things so that iaxc_video_format_set() and
 * probably several other iaxc_*() calls do not happen until after
 * iaxc_initialize().
 *
 * Once that happens, these members of video_format_info can be rolled
 * back into video_info and we can initialize the members in
 * video_initialize().
 */
static struct video_format_info vfinfo =
{
	320,    /* width */
	240,    /* height */
	15,     /* fps */
	150000, /* bitrate */
	1500,   /* fragsize */
	0,      /* format preferred */
	0,      /* format allowed */
};

extern int selected_call;
extern int test_mode;
extern struct iaxc_call * calls;

/* to prevent clearing a call while in capture callback */
extern __inline int try_iaxc_lock();
extern __inline void put_iaxc_lock();

EXPORT unsigned int iaxc_get_video_prefs(void)
{
	return vinfo.prefs;
}

static void reset_codec_stats(struct iaxc_video_codec *vcodec)
{
	if ( !vcodec )
		return;

	memset(&vcodec->video_stats, 0, sizeof(struct iaxc_video_stats));
	vcodec->video_stats.start_time = iax_tvnow();
}

static void reset_video_stats(struct iaxc_call * call)
{
	if ( !call )
		return;

	reset_codec_stats(call->vdecoder);
	reset_codec_stats(call->vencoder);
}

/* Collect and return video statistics. Also reset statistics if required.
 * Returns a pointer to the data.
 * Right now we use two different codecs for encoding and decoding. We need
 * to collate information from both and wrap it into one nice struct.
 */
static int get_stats(struct iaxc_call *call, struct iaxc_video_stats *stats,
		int reset)
{
	if ( !call || !stats )
		return -1;

	memset(stats, 0, sizeof(*stats));

	if ( call->vencoder )
	{
		stats->sent_slices = call->vencoder->video_stats.sent_slices;
		stats->acc_sent_size = call->vencoder->video_stats.acc_sent_size;
		stats->outbound_frames = call->vencoder->video_stats.outbound_frames;
		stats->avg_outbound_bps = call->vencoder->video_stats.avg_outbound_bps;
		stats->avg_outbound_fps = call->vencoder->video_stats.avg_outbound_fps;
	}

	if ( call->vdecoder )
	{
		stats->received_slices = call->vdecoder->video_stats.received_slices;
		stats->acc_recv_size = call->vdecoder->video_stats.acc_recv_size;
		stats->inbound_frames = call->vdecoder->video_stats.inbound_frames;
		stats->dropped_frames = call->vdecoder->video_stats.dropped_frames;
		stats->avg_inbound_bps = call->vdecoder->video_stats.avg_inbound_bps;
		stats->avg_inbound_fps = call->vdecoder->video_stats.avg_inbound_fps;
	}

	if ( reset )
		reset_video_stats(call);

	return 0;
}

static int maybe_send_stats(struct iaxc_call * call)
{
	const long video_stats_interval = 1000; /* milliseconds */
	static struct timeval video_stats_start = {0, 0};
	iaxc_event e;
	struct timeval now;

	if ( !call )
		return -1;

	if ( video_stats_start.tv_sec == 0 && video_stats_start.tv_usec == 0 )
		video_stats_start = iax_tvnow();

	now = iax_tvnow();

	if ( iaxci_msecdiff(&now, &video_stats_start) > video_stats_interval )
	{
		get_stats(call, &e.ev.videostats.stats, 1);
		e.type = IAXC_EVENT_VIDEOSTATS;
		e.ev.videostats.callNo = selected_call;
		iaxci_post_event(e);

		video_stats_start = now;
	}

	return 0;
}

/* TODO: The encode parameter to this function is unused within this
 * function. However, clients of this function still use this parameter.
 * What ends up happening is we instantiate the codec encoder/decoder
 * pairs multiple times. This seems not the best. For example, it is
 * not clear that all codecs are idempotent to the extent that they can
 * be initialized multiple times. Furthermore, within iaxclient itself,
 * we have a nasty habit of using global statically allocated data.
 * Multiple codec_video_*_new calls could easily result in race
 * conditions or more blatantly bad problems.
 *
 * Splitting encoder and decoder creation has some merit.
 *
 *  - Avoid allocating encoder or decoder resources when not needed.
 *  - Allows using different codecs for sending and receiving.
 *  - Allows different codec parameters for sending and receiving.
 *
 */
static struct iaxc_video_codec *create_codec(int format, int encode)
{
	struct iaxc_video_codec * vcodec = 0;

	iaxci_usermsg(IAXC_TEXT_TYPE_NOTICE, "Creating codec format 0x%x",
			format);

	switch ( format )
	{
	case IAXC_FORMAT_H261:
	case IAXC_FORMAT_H263:
	case IAXC_FORMAT_H263_PLUS:
	case IAXC_FORMAT_MPEG4:
	case IAXC_FORMAT_H264:
#ifdef USE_FFMPEG
		vcodec = codec_video_ffmpeg_new(format,
				vfinfo.width,
				vfinfo.height,
				vfinfo.framerate,
				vfinfo.bitrate,
				vfinfo.fragsize);
#endif
		break;

	case IAXC_FORMAT_THEORA:
#ifdef USE_THEORA
		vcodec = codec_video_theora_new(format,
				vfinfo.width,
				vfinfo.height,
				vfinfo.framerate,
				vfinfo.bitrate,
				vfinfo.fragsize);
#endif
		break;
	}

	reset_codec_stats(vcodec);

	return vcodec;
}

/*
 * Returns video data to the main application using the callback mechanism.
 * This function creates a dynamic copy of the video data. The memory is freed
 * in iaxci_post_event. This is because the event we post may be queued and the
 * frame data must live until after it is dequeued.

 \todo For encoded data, set the event format to the calls video format.
       For raw data, set the format to 0.

 \todo For encoded data, set the event format to the calls video format. For raw data, set the format to 0.
 */
int show_video_frame(const char * in_buf, int in_buf_size,
		int call_number, int source, int encoded,
		unsigned int timestamp_ms)
{
	iaxc_event e;
	char * buf = malloc(in_buf_size);

	assert(buf);
	assert(source == IAXC_SOURCE_REMOTE || source == IAXC_SOURCE_LOCAL);

	memcpy(buf, in_buf, in_buf_size);

	e.type = IAXC_EVENT_VIDEO;
	e.ev.video.ts = timestamp_ms;
	e.ev.video.data = buf;
	e.ev.video.size = in_buf_size;
	e.ev.video.format = vfinfo.format_preferred;
	e.ev.video.width = vinfo.width;
	e.ev.video.height = vinfo.height;
	e.ev.video.callNo = call_number;
	e.ev.video.encoded = encoded;
	e.ev.video.source = source;

	iaxci_post_event(e);

	return 0;
}

// Resize the buffer to 25% (half resolution on both w and h )
// No checks are made to ensure that the source dimensions are even numbers
static void quarter_rgb32(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst)
{
	int i;
	const unsigned char * src_even = src;
	const unsigned char * src_odd = src + src_w * 4;

	for ( i = 0 ; i < src_h / 2 ; i++ )
	{
		int j;
		for ( j = 0 ; j < src_w / 2 ; j++ )
		{
			short r, g, b;
			b = *src_even++;
			g = *src_even++;
			r = *src_even++;
			++src_even;

			b += *src_even++;
			g += *src_even++;
			r += *src_even++;
			++src_even;

			b += *src_odd++;
			g += *src_odd++;
			r += *src_odd++;
			++src_odd;

			b += *src_odd++;
			g += *src_odd++;
			r += *src_odd++;
			++src_odd;

			*dst++ = (unsigned char)(b >> 2);
			*dst++ = (unsigned char)(g >> 2);
			*dst++ = (unsigned char)(r >> 2);
			*dst++ = (unsigned char)0xff;
		}
		src_even = src_odd;
		src_odd += src_w * 4;
	}
}

// Resize the buffer to 25% (half resolution on both w and h )
// No checks are made to ensure that the source dimensions are even numbers
static void quarter_yuy2(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst)
{
	int i;
	const unsigned char * src_even = src;
	const unsigned char * src_odd = src + src_w * 2;

	for ( i = 0 ; i < src_h / 2 ; i++ )
	{
		int j;
		for ( j = 0 ; j < src_w / 4 ; j++ )
		{
			short y1, u, y2, v;
			y1  = *src_even++;
			u   = *src_even++;
			y1 += *src_even++;
			v   = *src_even++;

			y1 += *src_odd++;
			u  += *src_odd++;
			y1 += *src_odd++;
			v  += *src_odd++;

			y2  = *src_even++;
			u  += *src_even++;
			y2 += *src_even++;
			v  += *src_even++;

			y2 += *src_odd++;
			u  += *src_odd++;
			y2 += *src_odd++;
			v  += *src_odd++;

			*dst++ = (unsigned char)(y1 >> 2);
			*dst++ = (unsigned char)(u  >> 2);
			*dst++ = (unsigned char)(y2 >> 2);
			*dst++ = (unsigned char)(v  >> 2);
		}
		src_even = src_odd;
		src_odd += src_w * 2;
	}
}

// Resize the channel to 25% (half resolution on both w and h )
// No checks are made to ensure that the source dimensions are even numbers
static void quarter_channel(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst)
{
	int i;
	const unsigned char * evenl = src;
	const unsigned char * oddl = src + src_w;

	for ( i = 0 ; i < src_h / 2 ; i++ )
	{
		int j;
		for ( j = 0 ; j < src_w / 2 ; j++ )
		{
			int s = *(evenl++);
			s += *(evenl++);
			s += *(oddl++);
			s += *(oddl++);
			*(dst++) = (unsigned char)(s >> 2);
		}
		evenl = oddl;
		oddl += src_w;
	}
}

// Resize an I420 image by resizing each of the three channels.
// Destination buffer must be sufficiently large to accommodate
// the resulting image
static void resize_i420(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst, int dst_w, int dst_h)
{
	const unsigned char *src_u = src + src_w * src_h;
	const unsigned char *src_v = src_u + src_w * src_h / 4;
	unsigned char *dst_u = dst + dst_w * dst_h;
	unsigned char *dst_v = dst_u + dst_w * dst_h / 4;

	// Resize each channel separately
	if ( dst_w * 2 == src_w && dst_h * 2 == src_h )
	{
		quarter_channel(src, src_w, src_h, dst);
		quarter_channel(src_u, src_w / 2, src_h / 2, dst_u);
		quarter_channel(src_v, src_w / 2, src_h / 2, dst_v);
	}
/*
	else if ( dst_w * 4 == src_w && dst_h * 4 == src_h )
	{
		double_quarter_channel(src, src_w, src_h, dst);
		double_quarter_channel(src_u, src_w / 2, src_h / 2, dst_u);
		double_quarter_channel(src_v, src_w / 2, src_h / 2, dst_v);
	}
	else
	{
		resize_channel(src, src_w, src_h, dst, dst_w, dst_h);
		resize_channel(src_u, src_w / 2, src_h / 2,
				dst_u, dst_w / 2, dst_h / 2);
		resize_channel(src_v, src_w / 2, src_h / 2,
				dst_v, dst_w / 2, dst_h / 2);
	}
*/
}

// Resize an rgb32 image
// Destination buffer must be sufficiently large to accommodate
// the resulting image
static void resize_rgb32(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst, int dst_w, int dst_h)
{
	if ( dst_w * 2 == src_w && dst_h * 2 == src_h )
	{
		quarter_rgb32(src, src_w, src_h, dst);
	}
/*
	else if ( dst_w * 4 == src_w && dst_h * 4 == src_h )
	{
		double_quarter_rgb32(src, src_w, src_h, dst);
	}
	else
	{
		resize_rgb32_buffer(src, src_w, src_h, dst, dst_w, dst_h);
	}
*/
}

// Resize a yuy2 image.
// Destination buffer must be sufficiently large to accommodate
// the resulting image
static void resize_yuy2(const unsigned char *src, int src_w, int src_h,
		unsigned char *dst, int dst_w, int dst_h)
{
	if ( dst_w * 2 == src_w && dst_h * 2 == src_h )
	{
		quarter_yuy2(src, src_w, src_h, dst);
	}
/*
	else if ( dst_w * 4 == src_w && dst_h * 4 == src_h )
	{
		double_quarter_yuy2(src, src_w, src_h, dst);
	}
	else
	{
		resize_yuy2_buffer(src, src_w, src_h, dst, dst_w, dst_h);
	}
*/
}

static int video_device_notification_callback(vidcap_sapi *sapi,
			void * user_context)
{
	iaxc_event evt;

	if ( sapi != vinfo.sapi )
	{
		fprintf(stderr, "ERROR: wrong sapi in device notification\n");
		return -1;
	}

	/* notify application that device list has been updated */
	evt.type = IAXC_EVENT_VIDCAP_DEVICE;
	iaxci_post_event(evt);

	return 0;
}

static int capture_callback(vidcap_src * src, void * user_data,
		struct vidcap_capture_info * cap_info)
{
	static struct slice_set_t slice_set;

	/* The prefs may change inbetween capture callbacks, so we cannot
	 * precompute any of this prior to starting capture.
	 */
	const int send_encoded = !(vinfo.prefs & IAXC_VIDEO_PREF_SEND_DISABLE);
	const int recv_local_enc = vinfo.prefs & IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED;
	const int recv_local_raw = vinfo.prefs & IAXC_VIDEO_PREF_RECV_LOCAL_RAW;
	const int need_encode = send_encoded || recv_local_enc;
	const int local_rgb = vinfo.prefs & IAXC_VIDEO_PREF_RECV_RGB32;

	struct iaxc_call * call;
	struct timeval now;
	long time_delta;

	const char * rgb_buf = 0;
	int rgb_buf_size = 0;
	const char * i420_buf = 0;
	int i420_buf_size = 0;
	const char * source_buf = 0;
	int source_buf_size = 0;

	iaxc_event evt;

	int i;

	if ( cap_info->error_status )
	{
		fprintf(stderr, "vidcap capture error %d\n",
				cap_info->error_status);
		vinfo.capturing = 0;

		/* NOTE:
		 * We want to release now, but we're in the capture callback thread.
		 * vidcap_src_release() fails during capture.
		 *
		 * We'll defer the release until the end-application's main thread
		 * requires the release - if either iaxc_set_video_prefs(),
		 * iaxc_video_device_set() or video_destroy() are called.
		 */

		evt.type = IAXC_EVENT_VIDCAP_ERROR;
		iaxci_post_event(evt);
		return -1;
	}

	if ( cap_info->video_data_size < 1 )
	{
		fprintf(stderr, "FYI: callback with no data\n");
		return 0;
	}

	if ( vinfo.prefs & IAXC_VIDEO_PREF_CAPTURE_DISABLE )
		return 0;

	if ( vinfo.width != vinfo.fmt_info.width )
	{
		vinfo.scale_image((const unsigned char *)cap_info->video_data,
				vinfo.fmt_info.width,
				vinfo.fmt_info.height,
				(unsigned char *)vinfo.scaled_buf,
				vinfo.width,
				vinfo.height);

		source_buf = vinfo.scaled_buf;
		source_buf_size = vinfo.scaled_buf_size;
	}
	else
	{
		source_buf = cap_info->video_data;
		source_buf_size = cap_info->video_data_size;
	}

	if ( vinfo.convert_to_rgb32 && recv_local_raw && local_rgb )
	{
		vinfo.convert_to_rgb32(
				vinfo.width,
				vinfo.height,
				source_buf,
				vinfo.converted_rgb_buf);

		rgb_buf = vinfo.converted_rgb_buf;
		rgb_buf_size = vinfo.converted_rgb_buf_size;
	}
	else
	{
		rgb_buf = source_buf;
		rgb_buf_size = source_buf_size;
	}

	if ( vinfo.convert_to_i420 && (need_encode ||
				(recv_local_raw && !local_rgb)) )
	{
		vinfo.convert_to_i420(
				vinfo.width,
				vinfo.height,
				source_buf,
				vinfo.converted_i420_buf);

		i420_buf = vinfo.converted_i420_buf;
		i420_buf_size = vinfo.converted_i420_buf_size;
	}
	else
	{
		i420_buf = source_buf;
		i420_buf_size = source_buf_size;
	}

	if ( recv_local_raw )
	{
		show_video_frame(local_rgb ? rgb_buf : i420_buf,
				local_rgb ? rgb_buf_size : i420_buf_size,
				-1, /* local call number */
				IAXC_SOURCE_LOCAL,
				0,  /* not encoded */
				0); /* timestamp (ms) */
	}

	/* Don't block waiting for this lock. If the main thread has the lock
	 * for the purpose of dumping the call, it may request that video
	 * capture stop - which would block until this callback returned.
	 */
	if ( try_iaxc_lock() )
	{
		/* give it a second try */
		iaxc_millisleep(5);
		if ( try_iaxc_lock() )
		{
			fprintf(stderr, "skipping processing of a video frame\n");
			return 0;
		}
	}

	if ( selected_call < 0 )
		goto callback_done;

	call = &calls[selected_call];

	if ( !call || !(call->state & (IAXC_CALL_STATE_COMPLETE |
					IAXC_CALL_STATE_OUTGOING)) )
	{
		goto callback_done;
	}

	if ( call->vformat == 0 )
	{
		goto callback_failed;
	}

	if ( !need_encode )
	{
		/* Since we are neither sending out encoded video on the
		 * network nor to the application, we no longer need the
		 * codec instance.
		 */
		if ( call->vencoder )
		{
			fprintf(stderr, "destroying codec %s\n",
					call->vencoder->name);
			call->vencoder->destroy(call->vencoder);
			call->vencoder = 0;
		}

		goto callback_done;
	}
	else
	{
		if ( call->vencoder &&
				(call->vencoder->format != call->vformat ||
				 call->vencoder->params_changed) )
		{
			call->vencoder->destroy(call->vencoder);
			call->vencoder = 0;
		}

		if ( !call->vencoder )
		{
			if ( !(call->vencoder = create_codec(call->vformat, 1)) )
			{
				fprintf(stderr, "ERROR: failed to create codec "
						"for format 0x%08x\n",
						call->vformat);

				goto callback_failed;
			}

			fprintf(stderr, "created encoder codec %s\n",
					call->vencoder->name);
		}

		if ( call->vencoder->encode(call->vencoder,
					i420_buf_size, i420_buf,
					&slice_set) )
		{
			fprintf(stderr, "failed to encode captured video\n");
			goto callback_failed;
		}
	}

	/* Gather statistics */
	call->vencoder->video_stats.outbound_frames++;

	now = iax_tvnow();
	time_delta =
		iaxci_msecdiff(&now, &call->vencoder->video_stats.start_time);

	if ( time_delta > 0 )
		call->vencoder->video_stats.avg_outbound_fps =
			(float)call->vencoder->video_stats.outbound_frames *
			1000.0f / (float)time_delta;

	if ( !call->session )
	{
		fprintf(stderr, "not sending video to sessionless call\n");
		goto callback_failed;
	}

	for ( i = 0; i < slice_set.num_slices; ++i )
	{
		//Pass the encoded frame to the main app
		// \todo Fix the call number
		if ( vinfo.prefs & IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED )
		{
			show_video_frame(slice_set.data[i],
					slice_set.size[i],
					-1, /* local call number */
					IAXC_SOURCE_LOCAL,
					1,  /* encoded */
					0); /* timestamp */
		}

		if ( !(vinfo.prefs & IAXC_VIDEO_PREF_SEND_DISABLE) )
		{
			if ( iax_send_video_trunk(call->session, call->vformat,
						slice_set.data[i],
						slice_set.size[i],
						0, /* not fullframe */
						i) == -1)
			{
				fprintf(stderr, "failed sending slice call %d "
						"size %d\n", selected_call,
						slice_set.size[i]);
				goto callback_failed;
			}

			/* More statistics */
			call->vencoder->video_stats.sent_slices++;
			call->vencoder->video_stats.acc_sent_size +=
				slice_set.size[i];
			if ( time_delta > 0 )
				call->vencoder->video_stats.avg_outbound_bps =
					call->vencoder->video_stats.acc_sent_size *
					8000 / time_delta;
		}
	}

	maybe_send_stats(call);

callback_done:
	put_iaxc_lock();
	return 0;

callback_failed:
	put_iaxc_lock();
	return -1;
}

static int prepare_for_capture()
{
	static const int fourcc_list[] =
	{
		VIDCAP_FOURCC_I420,
		VIDCAP_FOURCC_YUY2,
		VIDCAP_FOURCC_RGB32
	};

	static const int fourcc_list_len = sizeof(fourcc_list) / sizeof(int);
	static const int max_factor = 2;
	int scale_factor;
	int i;

	vinfo.width = vfinfo.width;
	vinfo.height = vfinfo.height;
	vinfo.fmt_info.fps_numerator = vfinfo.framerate;
	vinfo.fmt_info.fps_denominator = 1;

	for ( scale_factor = 1; scale_factor <= max_factor; scale_factor *= 2 )
	{
		vinfo.fmt_info.width = vfinfo.width * scale_factor;
		vinfo.fmt_info.height = vfinfo.height * scale_factor;

		for ( i = 0; i < fourcc_list_len; ++i )
		{
			vinfo.fmt_info.fourcc = fourcc_list[i];

			if ( !vidcap_format_bind(vinfo.src, &vinfo.fmt_info) )
				break;
		}

		if ( i != fourcc_list_len )
			break;
	}

	if ( i == fourcc_list_len )
	{
		fprintf(stderr, "failed to bind format %dx%d %f fps\n",
				vinfo.fmt_info.width,
				vinfo.fmt_info.height,
				(float)vinfo.fmt_info.fps_numerator /
				(float)vinfo.fmt_info.fps_denominator);
		return -1;
	}

	/* Prepare various conversion buffers */

	if ( vinfo.converted_i420_buf )
	{
		free(vinfo.converted_i420_buf);
		vinfo.converted_i420_buf = 0;
	}

	if ( vinfo.converted_rgb_buf )
	{
		free(vinfo.converted_rgb_buf);
		vinfo.converted_rgb_buf = 0;
	}

	vinfo.converted_i420_buf_size =
		vinfo.width * vinfo.height * 3 / 2;

	vinfo.converted_rgb_buf_size =
		vinfo.width * vinfo.height * 4;

	if ( vinfo.fmt_info.fourcc != VIDCAP_FOURCC_RGB32 )
		vinfo.converted_rgb_buf = malloc(vinfo.converted_rgb_buf_size);

	if ( vinfo.fmt_info.fourcc != VIDCAP_FOURCC_I420 )
		vinfo.converted_i420_buf = malloc(vinfo.converted_i420_buf_size);

	switch ( vinfo.fmt_info.fourcc )
	{
	case VIDCAP_FOURCC_RGB32:
		vinfo.convert_to_i420 = vidcap_rgb32_to_i420;
		vinfo.convert_to_rgb32 = 0;
		vinfo.scale_image = 0;
		vinfo.scaled_buf = 0;
		if ( vinfo.width != vinfo.fmt_info.width )
		{
			vinfo.scale_image = resize_rgb32;
			vinfo.scaled_buf_size = vinfo.converted_rgb_buf_size;
			vinfo.scaled_buf = malloc(vinfo.scaled_buf_size);
			fprintf(stderr, "scaling rgb32 images by 1/%d\n",
					scale_factor);
		}
		if ( !vinfo.converted_i420_buf )
			return -1;
		break;
	case VIDCAP_FOURCC_I420:
		vinfo.convert_to_i420 = 0;
		vinfo.convert_to_rgb32 = vidcap_i420_to_rgb32;
		vinfo.scale_image = 0;
		vinfo.scaled_buf = 0;
		if ( vinfo.width != vinfo.fmt_info.width )
		{
			vinfo.scale_image = resize_i420;
			vinfo.scaled_buf_size = vinfo.converted_i420_buf_size;
			vinfo.scaled_buf = malloc(vinfo.scaled_buf_size);
			fprintf(stderr, "scaling i420 images by 1/%d\n",
					scale_factor);
		}
		if ( !vinfo.converted_rgb_buf )
			return -1;
		break;
	case VIDCAP_FOURCC_YUY2:
		vinfo.convert_to_i420 = vidcap_yuy2_to_i420;
		vinfo.convert_to_rgb32 = vidcap_yuy2_to_rgb32;
		vinfo.scale_image = 0;
		vinfo.scaled_buf = 0;
		if ( vinfo.width != vinfo.fmt_info.width )
		{
			vinfo.scale_image = resize_yuy2;
			vinfo.scaled_buf_size = vinfo.width *
					vinfo.height * 2;
			vinfo.scaled_buf = malloc(vinfo.scaled_buf_size);
			fprintf(stderr, "scaling yuy2 images by 1/%d\n",
					scale_factor);
		}
		if ( !vinfo.converted_rgb_buf || !vinfo.converted_i420_buf )
			return -1;
		break;
	default:
		fprintf(stderr, "do not know how to deal with vidcap "
				"fourcc '%s'\n",
				vidcap_fourcc_string_get(vinfo.fmt_info.fourcc));
		return -1;
	}

	if ( vinfo.scale_image && !vinfo.scaled_buf )
		return -1;

	return 0;
}

static int ensure_acquired(int dev_id)
{
	int dev_num;

	if ( !vinfo.src )
	{
		MUTEXLOCK(&vinfo.dev_list_lock);

		for ( dev_num = 0; dev_num < vinfo.device_count; dev_num++ )
		{
			if ( vinfo.devices[dev_num].id == dev_id )
				break;
		}

		if ( dev_num == vinfo.device_count )
		{
			MUTEXUNLOCK(&vinfo.dev_list_lock);
			fprintf(stderr, "invalid vidcap dev id: %d\n", dev_id);
			return -1;
		}

		if ( !(vinfo.src = vidcap_src_acquire(vinfo.sapi,
					&vinfo.vc_src_info[dev_num])) )
		{
			vinfo.src = 0;
			MUTEXUNLOCK(&vinfo.dev_list_lock);
			fprintf(stderr, "failed to acquire video source\n");
			return -1;
		}

		fprintf(stderr, "acquired vidcap source %s (%s)\n",
				vinfo.vc_src_info[dev_num].description,
				vinfo.vc_src_info[dev_num].identifier);

		MUTEXUNLOCK(&vinfo.dev_list_lock);
	}

	return 0;
}

EXPORT int iaxc_set_video_prefs(unsigned int prefs)
{
	const unsigned int prefs_mask =
		IAXC_VIDEO_PREF_RECV_LOCAL_RAW      |
		IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED  |
		IAXC_VIDEO_PREF_RECV_REMOTE_RAW     |
		IAXC_VIDEO_PREF_RECV_REMOTE_ENCODED |
		IAXC_VIDEO_PREF_SEND_DISABLE        |
		IAXC_VIDEO_PREF_RECV_RGB32          |
		IAXC_VIDEO_PREF_CAPTURE_DISABLE;
	int ret;

	if ( prefs & ~prefs_mask )
	{
		fprintf(stderr, "ERROR: unexpected video preference: 0x%0x\n",
				prefs);
		return -1;
	}

	vinfo.prefs = prefs;

	if ( test_mode )
		return 0;

	/* Not sending any video and not needing any form of
	 * local video implies that we do not need to capture
	 * video.
	 */
	if ( prefs & IAXC_VIDEO_PREF_CAPTURE_DISABLE ||
			((prefs & IAXC_VIDEO_PREF_SEND_DISABLE) &&
			 !(prefs & IAXC_VIDEO_PREF_RECV_LOCAL_RAW) &&
			 !(prefs & IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED)) )
	{
		MUTEXLOCK(&vinfo.camera_lock);
		if ( vinfo.capturing && vinfo.src &&
				vidcap_src_capture_stop(vinfo.src) )
		{
			fprintf(stderr, "failed vidcap_src_capture_stop\n");
		}

		if ( vinfo.src && vidcap_src_release(vinfo.src) )
		{
			fprintf(stderr, "failed to release a video source\n");
		}

		vinfo.src = 0;
		vinfo.capturing = 0;
		MUTEXUNLOCK(&vinfo.camera_lock);
	}
	else
	{
		MUTEXLOCK(&vinfo.camera_lock);
		if ( !vinfo.capturing )
		{
			if ( vinfo.selected_device_id < 0 )
			{
				MUTEXUNLOCK(&vinfo.camera_lock);
				return -1;
			}

			if ( ensure_acquired(vinfo.selected_device_id) )
			{
				MUTEXUNLOCK(&vinfo.camera_lock);
				return -1;
			}

			if ( prepare_for_capture() )
			{
				MUTEXUNLOCK(&vinfo.camera_lock);
				return -1;
			}

			if ( (ret = vidcap_src_capture_start(vinfo.src,
						capture_callback, 0)) )
			{
				MUTEXUNLOCK(&vinfo.camera_lock);
				fprintf(stderr, "failed to start video capture: %d\n",
						ret);
				return -1;
			}

			vinfo.capturing = 1;
		}
		MUTEXUNLOCK(&vinfo.camera_lock);
	}

	return 0;
}

EXPORT void iaxc_video_format_set_cap(int preferred, int allowed)
{
	vfinfo.format_preferred = preferred;
	vfinfo.format_allowed = allowed;
}

EXPORT void iaxc_video_format_get_cap(int *preferred, int *allowed)
{
	*preferred = vfinfo.format_preferred;
	*allowed = vfinfo.format_allowed;
}

EXPORT void iaxc_video_format_set(int preferred, int allowed, int framerate,
		int bitrate, int width, int height, int fs)
{
	int real_pref = 0;
	int real_allowed = 0;
#ifdef USE_FFMPEG
	int tmp_allowed;
	int i;
#endif

	// Make sure resolution is in range
	if ( width < IAXC_VIDEO_MIN_WIDTH )
		width = IAXC_VIDEO_MIN_WIDTH;
	else if ( width > IAXC_VIDEO_MAX_WIDTH )
		width = IAXC_VIDEO_MAX_WIDTH;

	if ( height < IAXC_VIDEO_MIN_HEIGHT )
		height = IAXC_VIDEO_MIN_HEIGHT;
	else if ( height > IAXC_VIDEO_MAX_HEIGHT )
		height = IAXC_VIDEO_MAX_HEIGHT;

	vfinfo.framerate = framerate;
	vfinfo.bitrate = bitrate;
	vfinfo.width = width;
	vfinfo.height = height;
	vfinfo.fragsize = fs;

	vfinfo.format_allowed = 0;
	vfinfo.format_preferred = 0;

	if ( preferred && (preferred & ~IAXC_VIDEO_FORMAT_MASK) )
	{
		fprintf(stderr, "ERROR: Preferred video format invalid.\n");
		preferred = 0;
	}

	/* This check:
	 * 1. Check if preferred is a supported and compiled in codec. If
	 *    not, switch to the default preferred format.
	 * 2. Check if allowed contains a list of all supported and compiled
	 *    in codec. If there are some unavailable codec, remove it from
	 *    this list.
	 */

	if ( preferred & IAXC_FORMAT_THEORA )
		real_pref = IAXC_FORMAT_THEORA;

#ifdef USE_FFMPEG
	if ( codec_video_ffmpeg_check_codec(preferred) )
		real_pref = preferred;
#endif

	if ( !real_pref )
	{
		/* If preferred codec is not available switch to the
		 * supported default codec.
		 */
		fprintf(stderr, "Preferred codec (0x%08x) is not available. "
				"Switching to default\n", preferred);
		real_pref = IAXC_FORMAT_THEORA;
	}

	/* Check on allowed codecs availability */

	if ( allowed & IAXC_FORMAT_THEORA )
		real_allowed |= IAXC_FORMAT_THEORA;

#ifdef USE_FFMPEG
	/* TODO: This codec_video_ffmpeg_check_codec stuff is bogus. We
	 * need a standard interface in our codec wrappers that allows us to
	 * (1) test if a selected codec is valid and/or (2) return the set of
	 * available valid codecs. With that, we should be able to come up
	 * with a more elegant algorithm here for determining the video codec.
	 */
	for ( i = 0; i <= 24; i++)
	{
		tmp_allowed = 1 << i;
		if ( (allowed & tmp_allowed)  &&
				 codec_video_ffmpeg_check_codec(tmp_allowed) )
			real_allowed |= tmp_allowed;
	}
#endif

	if ( !real_pref )
	{
		fprintf(stderr, "Audio-only client!\n");
	} else
	{
		vfinfo.format_preferred = real_pref;

		/*
		 * When a client use a 'preferred' format, it can force to
		 * use allowed formats using a non-zero value for 'allowed'
		 * parameter. If it is left 0, the client will use all
		 * capabilities set by default in this code.
		 */
		if ( real_allowed )
		{
			vfinfo.format_allowed = real_allowed;
		}
		else
		{
#ifdef USE_FFMPEG
			vfinfo.format_allowed |= IAXC_FORMAT_H263_PLUS
				| IAXC_FORMAT_H263
				| IAXC_FORMAT_MPEG4
				| IAXC_FORMAT_H264;
#endif
			vfinfo.format_allowed |= IAXC_FORMAT_THEORA;
		}
	}
}

void iaxc_video_params_change(int framerate, int bitrate, int width,
		int height, int fs)
{
	struct iaxc_call *call;

	/* set default video params */
	if ( framerate > 0 )
		vfinfo.framerate = framerate;
	if ( bitrate > 0 )
		vfinfo.bitrate = bitrate;
	if ( width > 0 )
		vfinfo.width = width;
	if ( height > 0 )
		vfinfo.height = height;
	if ( fs > 0 )
		vfinfo.fragsize = fs;

	if ( selected_call < 0 )
		return;

	call = &calls[selected_call];

	if ( !call || !call->vencoder )
		return;

	call->vencoder->params_changed = 1;

	if ( framerate > 0 )
		call->vencoder->framerate = framerate;
	if ( bitrate > 0 )
		call->vencoder->bitrate = bitrate;
	if ( width > 0 )
		call->vencoder->width = width;
	if ( height > 0 )
		call->vencoder->height = height;
	if ( fs > 0 )
		call->vencoder->fragsize = fs;
}

/* process an incoming video frame */
int video_recv_video(struct iaxc_call *call, int sel_call,
		void *encoded_video, int encoded_video_len,
		unsigned int ts, int format)
{
	enum
	{
		max_pixels = IAXC_VIDEO_MAX_WIDTH * IAXC_VIDEO_MAX_HEIGHT,
		max_yuv_buf_size = max_pixels * 3 / 2,
		max_rgb_buf_size = max_pixels * 4
	};

	static char yuv_buf[max_yuv_buf_size];
	static char rgb_buf[max_rgb_buf_size];

	const int pixels = vfinfo.width * vfinfo.height;
	const int yuv_size = pixels * 3 / 2;
	const int rgb_size = pixels * 4;

	int out_size = max_yuv_buf_size;
	int ret;
	struct timeval now;
	long time;

	if ( !call )
		return 0;

	if ( format == 0 )
	{
		fprintf(stderr, "video_recv_video: Format is zero (should't happen)!\n");
		return -1;
	}

	// Send the encoded frame to the main app if necessary
	if ( vinfo.prefs & IAXC_VIDEO_PREF_RECV_REMOTE_ENCODED )
	{
		show_video_frame((char *)encoded_video,
				encoded_video_len,
				-1,  /* TODO: why is the callnumber -1? */
				IAXC_SOURCE_REMOTE,
				1,   /* encoded */
				ts);
	}

	/* destroy vdecoder if it is incorrect type */
	if ( call->vdecoder && call->vdecoder->format != format )
	{
		call->vdecoder->destroy(call->vdecoder);
		call->vdecoder = NULL;
	}

	/* If the user does not need decoded video, then do not decode. */
	if ( !(vinfo.prefs & IAXC_VIDEO_PREF_RECV_REMOTE_RAW) )
		return 0;

	/* create decoder if necessary */
	if ( !call->vdecoder )
	{
		call->vdecoder = create_codec(format, 0);
		fprintf(stderr,"**** Created decoder codec %s\n",call->vdecoder->name);
	}

	if ( !call->vdecoder )
	{
		fprintf(stderr, "ERROR: Video codec could not be created: %d\n",
				format);
		return -1;
	}

	/* Statistics */
	now = iax_tvnow();
	time = iaxci_msecdiff(&now, &call->vdecoder->video_stats.start_time);
	call->vdecoder->video_stats.received_slices++;
	call->vdecoder->video_stats.acc_recv_size += encoded_video_len;
	if ( time > 0 )
		call->vdecoder->video_stats.avg_inbound_bps =
			call->vdecoder->video_stats.acc_recv_size * 8000 / time;

	ret = call->vdecoder->decode(call->vdecoder, encoded_video_len,
			(char *)encoded_video, &out_size, yuv_buf);

	if ( ret < 0 )
	{
		fprintf(stderr, "ERROR: decode error\n");
		return -1;
	}
	else if ( ret > 0 )
	{
		/* This indicates that a complete frame cannot yet
		 * be decoded. This is okay.
		 */
		return 0;
	}
	else if ( out_size != yuv_size )
	{
		fprintf(stderr, "ERROR: decoder returned unexpected sized "
				"frame (expected %d got %d)\n",
				yuv_size, out_size);
		return -1;
	}

	/* Statistics */
	call->vdecoder->video_stats.inbound_frames++;
	if ( time > 0 )
		call->vdecoder->video_stats.avg_inbound_fps =
			call->vdecoder->video_stats.inbound_frames *
			1000.0F / time;

	if ( vinfo.prefs & IAXC_VIDEO_PREF_RECV_RGB32 )
	{
		vidcap_i420_to_rgb32(vfinfo.width, vfinfo.height,
				yuv_buf, rgb_buf);

		show_video_frame(rgb_buf, rgb_size, sel_call,
				IAXC_SOURCE_REMOTE, 0, ts);
	}
	else
	{
		show_video_frame(yuv_buf, yuv_size, sel_call,
				IAXC_SOURCE_REMOTE, 0, ts);
	}

	return 0;
}

EXPORT int iaxc_video_devices_get(struct iaxc_video_device **devs,
			int *num_devs, int *id_selected)
{
	int new_device_count;
	int old_device_count;
	struct vidcap_src_info *new_src_list;
	struct vidcap_src_info *old_src_list;
	struct iaxc_video_device  *new_iaxc_dev_list;
	struct iaxc_video_device  *old_iaxc_dev_list;
	int found_selected_device = 0;
	int list_changed;
	int i, n;

	if ( !vinfo.sapi )
		return -1;

	/* update libvidcap's device list */
	new_device_count = vidcap_src_list_update(vinfo.sapi);
	list_changed = new_device_count != vinfo.device_count;

	if ( new_device_count < 0 )
	{
		fprintf(stderr, "ERROR: failed getting updated vidcap device list: %d\n",
				new_device_count);
		return -1;
	}

	new_src_list = calloc(new_device_count, sizeof(struct vidcap_src_info));

	if ( !new_src_list )
	{
		fprintf(stderr, "ERROR: failed updated source allocation\n");
		return -1;
	}

	new_iaxc_dev_list = calloc(new_device_count,
			sizeof(struct iaxc_video_device));

	if ( !new_iaxc_dev_list )
	{
		free(new_src_list);
		fprintf(stderr, "ERROR: failed source allocation update\n");
		return -1;
	}

	/* get an updated libvidcap device list */
	if ( vidcap_src_list_get(vinfo.sapi, new_device_count, new_src_list) )
	{
		fprintf(stderr, "ERROR: failed vidcap_srcList_get().\n");

		free(new_src_list);
		free(new_iaxc_dev_list);
		return -1;
	}

	/* build a new iaxclient video source list */
	for ( n = 0; n < new_device_count; n++ )
	{
		new_iaxc_dev_list[n].name = strdup(new_src_list[n].description);
		new_iaxc_dev_list[n].id_string = strdup(new_src_list[n].identifier);

		/* This device may have been here all along.
		 * If it has, re-assign that device id
		 * else assign a new id
		 */
		for ( i = 0; i < vinfo.device_count; i++ )
		{
			/* check both the device name and its unique identifier */
			if ( !strcmp(new_iaxc_dev_list[n].name, vinfo.devices[i].name) &&
					!strcmp(new_iaxc_dev_list[n].id_string,
							vinfo.devices[i].id_string) )
			{
				new_iaxc_dev_list[n].id = vinfo.devices[i].id;

				if ( vinfo.selected_device_id == new_iaxc_dev_list[n].id )
					found_selected_device = 1;
				break;
			}
		}
		if ( i == vinfo.device_count )
		{
			new_iaxc_dev_list[n].id = vinfo.next_id++;

			list_changed = 1;
		}
	}

	if ( !list_changed )
	{
		/* Free new lists. Nothing's really changed */
		free(new_src_list);
		for ( i = 0; i < new_device_count; i++ )
		{
			free((void *)new_iaxc_dev_list[i].name);
			free((void *)new_iaxc_dev_list[i].id_string);
		}
		free(new_iaxc_dev_list);
	}
	else
	{
		old_device_count = vinfo.device_count;
		old_src_list = vinfo.vc_src_info;
		old_iaxc_dev_list = vinfo.devices;

		/* Update iaxclient's device list info               */
		/* Lock since other iaxclient funcs use these fields */
		MUTEXLOCK(&vinfo.dev_list_lock);

		vinfo.device_count = new_device_count;
		vinfo.vc_src_info = new_src_list;
		vinfo.devices = new_iaxc_dev_list;

		MUTEXUNLOCK(&vinfo.dev_list_lock);

		/* free old lists */
		free(old_src_list);
		for ( i = 0; i < old_device_count; i++ )
		{
			free((void *)old_iaxc_dev_list[i].name);
			free((void *)old_iaxc_dev_list[i].id_string);
		}
		free(old_iaxc_dev_list);
	}

	/* If we can't find the selected device, defer releasing it.
	 * It'll happen when we set a new device, or shutdown
	 */

	if ( !found_selected_device )
		vinfo.selected_device_id = -1;

	*devs = vinfo.devices;
	*num_devs = vinfo.device_count;
	*id_selected = vinfo.selected_device_id;

	return list_changed;
}

EXPORT int iaxc_video_device_set(int capture_dev_id)
{
	int ret = 0;
	int dev_num = 0;

	MUTEXLOCK(&vinfo.camera_lock);

	if ( capture_dev_id == vinfo.selected_device_id )
	{
		MUTEXUNLOCK(&vinfo.camera_lock);
		return 0;
	}

	if ( capture_dev_id < 0 )
	{
		MUTEXUNLOCK(&vinfo.camera_lock);
		fprintf(stderr, "invalid video device id ( < 0 )\n");
		return -1;
	}

	MUTEXLOCK(&vinfo.dev_list_lock);

	for ( dev_num = 0; dev_num < vinfo.device_count; dev_num++ )
	{
		if ( vinfo.devices[dev_num].id == capture_dev_id )
			break;
	}

	if ( dev_num == vinfo.device_count )
	{
		MUTEXUNLOCK(&vinfo.dev_list_lock);
		MUTEXUNLOCK(&vinfo.camera_lock);
		fprintf(stderr, "invalid video device id: %d\n",
				capture_dev_id);
		return -1;
	}

	vinfo.selected_device_id = capture_dev_id;

	if ( vinfo.capturing && vinfo.src &&
			vidcap_src_capture_stop(vinfo.src) )
	{
		fprintf(stderr, "failed to stop video capture\n");
	}

	if ( vinfo.src && vidcap_src_release(vinfo.src) )
	{
		fprintf(stderr, "failed to release video source\n");
	}

	vinfo.src = 0;

	MUTEXUNLOCK(&vinfo.dev_list_lock);

	if ( vinfo.capturing )
	{
		if ( ensure_acquired(capture_dev_id) )
		{
			MUTEXUNLOCK(&vinfo.camera_lock);
			return -1;
		}

		if ( prepare_for_capture() )
		{
			MUTEXUNLOCK(&vinfo.camera_lock);
			return -1;
		}

		if ( (ret = vidcap_src_capture_start(vinfo.src,
						capture_callback, 0)) )
		{
			MUTEXUNLOCK(&vinfo.camera_lock);
			fprintf(stderr, "failed to restart video capture: %d\n", ret);
			return -1;
		}
	}

	MUTEXUNLOCK(&vinfo.camera_lock);

	return 0;
}

int video_initialize(void)
{
	int i;
	const int starting_id = 50;

	memset(&vinfo, 0, sizeof(vinfo));

	vinfo.width = vfinfo.width;
	vinfo.height = vfinfo.height;
	vinfo.selected_device_id = -1;
	vinfo.next_id = starting_id;

	MUTEXINIT(&vinfo.camera_lock);
	MUTEXINIT(&vinfo.dev_list_lock);

	if ( !(vinfo.vc = vidcap_initialize()) )
	{
		fprintf(stderr, "ERROR: failed vidcap_initialize\n");
		return -1;
	}

	if ( !(vinfo.sapi = vidcap_sapi_acquire(vinfo.vc, 0)) )
	{
		fprintf(stderr, "ERROR: failed vidcap_sapi_acquire\n");
		goto bail;
	}

	if ( vidcap_sapi_info_get(vinfo.sapi, &vinfo.sapi_info) )
	{
		fprintf(stderr, "ERROR: failed vidcap_sapi_info_get\n");
		goto bail;
	}

	fprintf(stderr, "using vidcap sapi %s (%s)\n",
			vinfo.sapi_info.description,
			vinfo.sapi_info.identifier);

	vinfo.device_count = vidcap_src_list_update(vinfo.sapi);
	if ( vinfo.device_count < 0 )
	{
		fprintf(stderr,
				"ERROR: failed updating video capture devices list\n");
		goto bail;
	}

	vinfo.vc_src_info = calloc(vinfo.device_count,
			sizeof(struct vidcap_src_info));

	if ( !vinfo.vc_src_info )
	{
		fprintf(stderr, "ERROR: failed vinfo field allocations\n");
		goto bail;
	}

	vinfo.devices = calloc(vinfo.device_count,
			sizeof(struct iaxc_video_device));

	if ( !vinfo.devices )
	{
		fprintf(stderr, "ERROR: failed vinfo field allocation\n");
		free(vinfo.vc_src_info);
		goto bail;
	}

	if ( vidcap_src_list_get(vinfo.sapi, vinfo.device_count,
				vinfo.vc_src_info) )
	{
		fprintf(stderr, "ERROR: failed vidcap_src_list_get()\n");
		free(vinfo.vc_src_info);
		free(vinfo.devices);
		goto bail;
	}

	/* build initial iaxclient video source list */
	for ( i = 0; i < vinfo.device_count; i++ )
	{
		vinfo.devices[i].name = strdup(vinfo.vc_src_info[i].description);
		vinfo.devices[i].id_string = strdup(vinfo.vc_src_info[i].identifier);
		/* Let's be clear that the device id is not some
		 * base-zero index. Once plug-n-play is implemented,
		 * these ids may diverge as devices are added
		 * and removed.
		 */
		vinfo.devices[i].id = vinfo.next_id++;
	}

	if ( vinfo.device_count )
	{
		/* set default source - the first device */
		iaxc_video_device_set(vinfo.devices[0].id);
	}

	/* setup device notification callback
	 * for device insertion and removal
	 */
	if ( vidcap_srcs_notify(vinfo.sapi, &video_device_notification_callback, 0) )
	{
		fprintf(stderr, "ERROR: failed vidcap_srcs_notify()\n");
		goto late_bail;
	}

	vinfo.prefs =
		IAXC_VIDEO_PREF_RECV_LOCAL_RAW |
		IAXC_VIDEO_PREF_RECV_REMOTE_RAW |
		IAXC_VIDEO_PREF_CAPTURE_DISABLE;

	return 0;

late_bail:
	free(vinfo.vc_src_info);

	for ( i = 0; i < vinfo.device_count; i++ )
	{
		free((void *)vinfo.devices[i].name);
		free((void *)vinfo.devices[i].id_string);
	}

	free(vinfo.devices);

bail:
	if ( vinfo.sapi )
	{
		vidcap_sapi_release(vinfo.sapi);
		vinfo.sapi = 0;
	}
	vidcap_destroy(vinfo.vc);
	vinfo.vc = 0;
	return -1;
}

int video_destroy(void)
{
	int i;

	if ( !vinfo.vc )
		return -1;

	free(vinfo.vc_src_info);
	for ( i = 0; i < vinfo.device_count; i++ )
	{
		free((void *)vinfo.devices[i].name);
		free((void *)vinfo.devices[i].id_string);
	}
	free(vinfo.devices);

	if ( vinfo.capturing && vinfo.src )
		vidcap_src_capture_stop(vinfo.src);

	if ( vinfo.src )
		vidcap_src_release(vinfo.src);

	vidcap_destroy(vinfo.vc);
	vinfo.vc = 0;

	MUTEXDESTROY(&vinfo.camera_lock);
	MUTEXDESTROY(&vinfo.dev_list_lock);

	if ( vinfo.converted_i420_buf )
	{
		free(vinfo.converted_i420_buf);
		vinfo.converted_i420_buf = 0;
	}

	if ( vinfo.converted_rgb_buf )
	{
		free(vinfo.converted_rgb_buf);
		vinfo.converted_rgb_buf = 0;
	}

	if ( vinfo.scaled_buf )
	{
		free(vinfo.scaled_buf);
		vinfo.scaled_buf = 0;
	}

	return 0;
}

void iaxc_YUV420_to_RGB32(int width, int height, const char * src, char * dst)
{
	if ( vidcap_i420_to_rgb32(width, height, src, dst) )
		iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
				"failed iaxc_YUV420_to_RGB32()");
}

int iaxc_is_camera_working()
{
	/* This tells us how many video sources are available, so
	 * we are saying that the "camera is working" if there exists
	 * more than zero cameras.
	 */
	return vinfo.sapi && vidcap_src_list_update(vinfo.sapi) > 0;
}

int video_send_stats(struct iaxc_call * call)
{
	const long video_stats_interval = 1000; /* milliseconds */
	static struct timeval video_stats_start = {0, 0};
	iaxc_event e;
	struct timeval now;

	if ( !call )
		return -1;

	if ( video_stats_start.tv_sec == 0 && video_stats_start.tv_usec == 0 )
		video_stats_start = iax_tvnow();

	now = iax_tvnow();

	if ( iaxci_msecdiff(&now, &video_stats_start) > video_stats_interval )
	{
		get_stats(call, &e.ev.videostats.stats, 1);
		e.type = IAXC_EVENT_VIDEOSTATS;
		e.ev.videostats.callNo = selected_call;
		iaxci_post_event(e);

		video_stats_start = now;
	}

	return 0;
}

EXPORT int iaxc_push_video(void *data, unsigned int size, int fragment)
{
	struct iaxc_call *call;

	if (selected_call < 0)
		return -1;

	call = &calls[selected_call];

	if ( vinfo.prefs & IAXC_VIDEO_PREF_SEND_DISABLE )
		return 0;

	//fprintf(stderr, "iaxc_push_video: sending video size %d\n", size);

	// Fragment if needed
	if ( fragment )
	{
		static struct slice_set_t slice_set;
		int i;

		if ( !vinfo.sc )
			vinfo.sc = create_slicer_context((unsigned short)rand(),
					vfinfo.fragsize);

		slice(data, size, &slice_set, vinfo.sc);
		for ( i = 0 ; i < slice_set.num_slices ; i++ )
		{
			if ( iax_send_video_trunk(call->session,
			                          call->vformat,
			                          slice_set.data[i],
			                          slice_set.size[i],
			                          0,
			                          i
			                         ) == -1
			   )
			{
				fprintf(stderr, "Failed to send a slice, call %d, size %d: %s\n",
								selected_call, slice_set.size[i], iax_errstr);
				return -1;
			}

		}
	} else
	{
		if ( iax_send_video_trunk(call->session, call->vformat, data,
					size, 0, 0) == -1 )
		{
			fprintf(stderr, "iaxc_push_video: failed to send "
					"video frame of size %d on call %d: %s\n",
					size, selected_call, iax_errstr);
			return -1;
		}
	}

	return 0;
}

