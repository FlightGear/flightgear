/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Michael Van Donselaar <mvand@vandonselaar.org>
 * Shawn Lawrence <shawn.lawrence@terracecomm.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#include "audio_encode.h"
#include "iaxclient_lib.h"
#include "iax-client.h"
#ifdef CODEC_GSM
#include "codec_gsm.h"
#endif
#include "codec_ulaw.h"
#include "codec_alaw.h"
#include "codec_speex.h"
#include <speex/speex_preprocess.h>

#ifdef CODEC_ILBC
#include "codec_ilbc.h"
#endif

float iaxci_silence_threshold = AUDIO_ENCODE_SILENCE_DB;

static float input_level = 0.0f;
static float output_level = 0.0f;

static SpeexPreprocessState *st = NULL;
static int speex_state_size = 0;
static int speex_state_rate = 0;
int iaxci_filters = IAXC_FILTER_AGC|IAXC_FILTER_DENOISE|IAXC_FILTER_AAGC|IAXC_FILTER_CN;

/* use to measure time since last audio was processed */
static struct timeval timeLastInput ;
static struct timeval timeLastOutput ;

static struct iaxc_speex_settings speex_settings =
{
	1,    /* decode_enhance */
	-1,   /* float quality */
	-1,   /* bitrate */
	0,    /* vbr */
	0,    /* abr */
	3     /* complexity */
};


static float vol_to_db(float vol)
{
	/* avoid calling log10() on zero which yields inf or
	 * negative numbers which yield nan */
	if ( vol <= 0.0f )
		return AUDIO_ENCODE_SILENCE_DB;
	else
		return log10f(vol) * 20.0f;
}

static int do_level_callback()
{
	static struct timeval last = {0,0};
	struct timeval now;
	float input_db;
	float output_db;

	now = iax_tvnow();

	if ( last.tv_sec != 0 && iaxci_usecdiff(&now, &last) < 100000 )
		return 0;

	last = now;

	/* if input has not been processed in the last second, set to silent */
	input_db = iaxci_usecdiff(&now, &timeLastInput) < 1000000 ?
			vol_to_db(input_level) : AUDIO_ENCODE_SILENCE_DB;

	/* if output has not been processed in the last second, set to silent */
	output_db = iaxci_usecdiff(&now, &timeLastOutput) < 1000000 ?
		vol_to_db(output_level) : AUDIO_ENCODE_SILENCE_DB;

	iaxci_do_levels_callback(input_db, output_db);

	return 0;
}

static void set_speex_filters()
{
	int i;

	if ( !st )
		return;

	i = 1; /* always make VAD decision */
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &i);
	i = (iaxci_filters & IAXC_FILTER_AGC) ? 1 : 0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &i);
	i = (iaxci_filters & IAXC_FILTER_DENOISE) ? 1 : 0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);

	/*
	* We can tweak these parameters to play with VAD sensitivity.
	* For now, we use the default values since it seems they are a good starting point.
	* However, if need be, this is the code that needs to change
	*/
	i = 35;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_START, &i);
	i = 20;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &i);
}

static void calculate_level(short *audio, int len, float *level)
{
	int big_sample = 0;
	int i;

	for ( i = 0; i < len; i++ )
	{
		const int sample = abs(audio[i]);
		big_sample = sample > big_sample ?
			sample : big_sample;
	}

	*level += ((float)big_sample / 32767.0f - *level) / 5.0f;
}

static int input_postprocess(void *audio, int len, int rate)
{
	static float lowest_volume = 1.0f;
	float volume;
	int silent = 0;

	if ( !st || speex_state_size != len || speex_state_rate != rate )
	{
		if (st)
			speex_preprocess_state_destroy(st);
		st = speex_preprocess_state_init(len,rate);
		speex_state_size = len;
		speex_state_rate = rate;
		set_speex_filters();
	}

	calculate_level((short *)audio, len, &input_level);

	/* only preprocess if we're interested in VAD, AGC, or DENOISE */
	if ( (iaxci_filters & (IAXC_FILTER_DENOISE | IAXC_FILTER_AGC)) ||
			iaxci_silence_threshold > 0.0f )
		silent = !speex_preprocess(st, (spx_int16_t *)audio, NULL);

	/* Analog AGC: Bring speex AGC gain out to mixer, with lots of hysteresis */
	/* use a higher continuation threshold for AAGC than for VAD itself */
	if ( !silent &&
	     iaxci_silence_threshold != 0.0f &&
	     (iaxci_filters & IAXC_FILTER_AGC) &&
	     (iaxci_filters & IAXC_FILTER_AAGC)
	   )
	{
		static int i = 0;

		i++;

		if ( (i & 0x3f) == 0 )
		{
			float loudness;
			speex_preprocess_ctl(st, SPEEX_PREPROCESS_GET_AGC_LOUDNESS, &loudness);
			if ( loudness > 8000.0f || loudness < 4000.0f )
			{
				const float level = iaxc_input_level_get();

				if ( loudness > 16000.0f && level > 0.5f )
				{
					/* lower quickly if we're really too hot */
					iaxc_input_level_set(level - 0.2f);
				}
				else if ( loudness > 8000.0f && level >= 0.15f )
				{
					/* lower less quickly if we're a bit too hot */
					iaxc_input_level_set(level - 0.1f);
				}
				else if ( loudness < 4000.0f && level <= 0.9f )
				{
					/* raise slowly if we're cold */
					iaxc_input_level_set(level + 0.1f);
				}
			}
		}
	}

	/* This is ugly. Basically just don't get volume level if speex thought
	 * we were silent. Just set it to 0 in that case */
	if ( iaxci_silence_threshold > 0.0f && silent )
		input_level = 0.0f;

	do_level_callback();

	volume = vol_to_db(input_level);

	if ( volume < lowest_volume )
		lowest_volume = volume;

	if ( iaxci_silence_threshold > 0.0f )
		return silent;
	else
		return volume < iaxci_silence_threshold;
}

static int output_postprocess(void *audio, int len)
{
	calculate_level((short *)audio, len, &output_level);

	do_level_callback();

	return 0;
}

static struct iaxc_audio_codec *create_codec(int format)
{
	switch (format & IAXC_AUDIO_FORMAT_MASK)
	{
#ifdef CODEC_GSM
	case IAXC_FORMAT_GSM:
		return codec_audio_gsm_new();
#endif
	case IAXC_FORMAT_ULAW:
		return codec_audio_ulaw_new();
	case IAXC_FORMAT_ALAW:
		return codec_audio_alaw_new();
	case IAXC_FORMAT_SPEEX:
		return codec_audio_speex_new(&speex_settings);
#ifdef CODEC_ILBC
	case IAXC_FORMAT_ILBC:
		return codec_audio_ilbc_new();
#endif
	default:
		/* ERROR: codec not supported */
		fprintf(stderr, "ERROR: Codec not supported: %d\n", format);
		return NULL;
	}
}

EXPORT void iaxc_set_speex_settings(int decode_enhance, float quality,
		int bitrate, int vbr, int abr, int complexity)
{
	speex_settings.decode_enhance = decode_enhance;
	speex_settings.quality = quality;
	speex_settings.bitrate = bitrate;
	speex_settings.vbr = vbr;
	speex_settings.abr = abr;
	speex_settings.complexity = complexity;
}

int audio_send_encoded_audio(struct iaxc_call *call, int callNo, void *data,
		int format, int samples)
{
	unsigned char outbuf[1024];
	int outsize = 1024;
	int silent;
	int insize = samples;

	/* update last input timestamp */
	timeLastInput = iax_tvnow();

	silent = input_postprocess(data, insize, 8000);

	if(silent)
	{
		if(!call->tx_silent)
		{  /* send a Comfort Noise Frame */
			call->tx_silent = 1;
			if ( iaxci_filters & IAXC_FILTER_CN )
				iax_send_cng(call->session, 10, NULL, 0);
		}
		return 0;  /* poof! no encoding! */
	}

	/* we're going to send voice now */
	call->tx_silent = 0;

	/* destroy encoder if it is incorrect type */
	if(call->encoder && call->encoder->format != format)
	{
		call->encoder->destroy(call->encoder);
		call->encoder = NULL;
	}

	/* just break early if there's no format defined: this happens for the
	 * first couple of frames of new calls */
	if(format == 0) return 0;

	/* create encoder if necessary */
	if(!call->encoder)
	{
		call->encoder = create_codec(format);
	}

	if(!call->encoder)
	{
		/* ERROR: no codec */
		fprintf(stderr, "ERROR: Codec could not be created: %d\n", format);
		return 0;
	}

	if(call->encoder->encode(call->encoder, &insize, (short *)data,
				&outsize, outbuf))
	{
		/* ERROR: codec error */
		fprintf(stderr, "ERROR: encode error: %d\n", format);
		return 0;
	}

	if(samples-insize == 0)
	{
		fprintf(stderr, "ERROR encoding (no samples output (samples=%d)\n", samples);
		return -1;
	}

	// Send the encoded audio data back to the app if required
	// TODO: fix the stupid way in which the encoded audio size is returned
	if ( iaxc_get_audio_prefs() & IAXC_AUDIO_PREF_RECV_LOCAL_ENCODED )
		iaxci_do_audio_callback(callNo, 0, IAXC_SOURCE_LOCAL, 1,
				call->encoder->format & IAXC_AUDIO_FORMAT_MASK,
				sizeof(outbuf) - outsize, outbuf);

	if(iax_send_voice(call->session,format, outbuf,
				sizeof(outbuf) - outsize, samples-insize) == -1)
	{
		fprintf(stderr, "Failed to send voice! %s\n", iax_errstr);
		return -1;
	}

	return 0;
}

/* decode encoded audio; return the number of bytes decoded
 * negative indicates error */
int audio_decode_audio(struct iaxc_call * call, void * out, void * data, int len,
		int format, int * samples)
{
	int insize = len;
	int outsize = *samples;

	timeLastOutput = iax_tvnow();

	if ( format == 0 )
	{
		fprintf(stderr, "audio_decode_audio: Format is zero (should't happen)!\n");
		return -1;
	}

	/* destroy decoder if it is incorrect type */
	if ( call->decoder && call->decoder->format != format )
	{
		call->decoder->destroy(call->decoder);
		call->decoder = NULL;
	}

	/* create decoder if necessary */
	if ( !call->decoder )
	{
		call->decoder = create_codec(format);
	}

	if ( !call->decoder )
	{
		fprintf(stderr, "ERROR: Codec could not be created: %d\n",
				format);
		return -1;
	}

	if ( call->decoder->decode(call->decoder,
				&insize, (unsigned char *)data,
				&outsize, (short *)out) )
	{
		fprintf(stderr, "ERROR: decode error: %d\n", format);
		return -1;
	}

	output_postprocess(out, *samples - outsize);

	*samples = outsize;
	return len - insize;
}

EXPORT int iaxc_get_filters(void)
{
	return iaxci_filters;
}

EXPORT void iaxc_set_filters(int filters)
{
	iaxci_filters = filters;
	set_speex_filters();
}

EXPORT void iaxc_set_silence_threshold(float thr)
{
	iaxci_silence_threshold = thr;
	set_speex_filters();
}

