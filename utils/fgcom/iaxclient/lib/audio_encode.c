/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003 HorizonLive.com, (c) 2004, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Michael Van Donselaar <mvand@vandonselaar.org> 
 * Shawn Lawrence <shawn.lawrence@terracecomm.com>
 *
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

#include "iaxclient_lib.h"
#include "codec_gsm.h"
#include "codec_ulaw.h"
#include "codec_alaw.h"
#include "codec_speex.h"

#ifdef CODEC_ILBC
	#include "codec_ilbc.h"
#endif

double iaxc_silence_threshold = -9e99;

static double input_level = 0, output_level = 0;

static SpeexPreprocessState *st = NULL;
static int speex_state_size = 0;
static int speex_state_rate = 0;
int    iaxc_filters = IAXC_FILTER_AGC|IAXC_FILTER_DENOISE|IAXC_FILTER_AAGC|IAXC_FILTER_CN;

/* use to measure time since last audio was processed */
static struct timeval timeLastInput ;
static struct timeval timeLastOutput ;

static struct iaxc_speex_settings speex_settings = {
  1,    /* decode_enhance */
  -1,   /* float quality */
  -1,   /* bitrate */
  0, 0, /* vbr, abr */
  3     /* complexity */
};

static double vol_to_db(double vol)
{
    /* avoid calling log10 on zero */
    return log10(vol + 1.0e-99) * 20;
}

/* just get the current input/output volumes, and return them. */
int iaxc_get_inout_volumes(int *input, int *output) {
  if(input) 
    *input = (int)vol_to_db(input_level);
  if(output)
    *output = (int)vol_to_db(output_level);

  return 0;
}

static int do_level_callback()
{
    static struct timeval last = {0,0};
    struct timeval now;
	double input_db;
	double output_db;

    gettimeofday(&now,NULL); 
    if(last.tv_sec != 0 && iaxc_usecdiff(&now,&last) < 100000) return 0;

    last = now;

	/* if input has not been processed in the last second, set to silent */
	input_db = ( iaxc_usecdiff( &now, &timeLastInput ) < 1000000 ) 
		? vol_to_db( input_level ) : -99.9;

	/* if output has not been processed in the last second, set to silent */
	output_db = ( iaxc_usecdiff( &now, &timeLastOutput ) < 1000000 ) 
		? vol_to_db( output_level ) : -99.9;

    iaxc_do_levels_callback((float) input_db, (float) output_db);

    return 0;
}


void iaxc_set_speex_filters() 
{
    int i;
    float f;

    if(!st) return;

    i = 1; /* always make VAD decision */
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &i);
    i = (iaxc_filters & IAXC_FILTER_AGC) ? 1 : 0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &i);
    i = (iaxc_filters & IAXC_FILTER_DENOISE) ? 1 : 0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);

    /* make vad more sensitive */
    f=0.30f;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_START, &f);
    f=0.07f;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &f);
}

static void calculate_level(short *audio, int len, double *level) {
    short now = 0;
    //double nowd;
    int i;

    for(i=0;i<len;i++)
      if(abs(audio[i]) > now) now = abs(audio[i]); 

    //nowd = now/32767; 

    *level += (((double)now/32767) - *level) / 5;
}

int iaxc_input_postprocess(void *audio, int len, int rate)
{
    double volume;
    static double lowest_volume = 1;
    int silent=0;

    if(!st || (speex_state_size != len) || (speex_state_rate != rate)) {
	if(st) speex_preprocess_state_destroy(st);
	st = speex_preprocess_state_init(len,rate);
	speex_state_size = len;
	speex_state_rate = rate;
	iaxc_set_speex_filters();
    }

    calculate_level(audio, len, &input_level);

    /* only preprocess if we're interested in VAD, AGC, or DENOISE */
    if((iaxc_filters & (IAXC_FILTER_DENOISE | IAXC_FILTER_AGC)) || iaxc_silence_threshold > 0)
	silent = !speex_preprocess(st, audio, NULL);

    /* Analog AGC: Bring speex AGC gain out to mixer, with lots of hysteresis */
    /* use a higher continuation threshold for AAGC than for VAD itself */
    if(!silent && 
	(iaxc_silence_threshold != 0) &&
	(iaxc_filters & IAXC_FILTER_AGC) && 
	(iaxc_filters & IAXC_FILTER_AAGC) && 
	(st->speech_prob > .20)
	) {
      static int i;
      double level;


      i++;

      if((i&0x3f) == 0) {
	  float loudness = st->loudness2;
	  if((loudness > 8000) || (loudness < 4000)) {
	    level =  iaxc_input_level_get();
	    /* fprintf(stderr, "loudness = %f, level = %f\n", loudness, level); */
	    /* lower quickly if we're really too hot */
	     if((loudness > 16000) && (level > 0.5)) {
		   /* fprintf(stderr, "lowering quickly level\n"); */
		   iaxc_input_level_set(level - 0.2);
	     } 
	     /* lower less quickly if we're a bit too hot */
	     else if((loudness > 8000) && (level >= 0.15)) {
		   /* fprintf(stderr, "lowering slowly level\n"); */
		   iaxc_input_level_set(level - 0.1);
	     }
	     /* raise slowly if we're cold */
	     else if((loudness < 4000) && (level <= 0.9)) {
		   /* fprintf(stderr, "raising level\n"); */
		   iaxc_input_level_set(level + 0.1);
	     }
	  }
      }
    }


    /* this is ugly.  Basically just don't get volume level if speex thought
     * we were silent.  just set it to 0 in that case */
    if(iaxc_silence_threshold > 0 && silent)
	input_level = 0;

    do_level_callback();

    volume = vol_to_db(input_level);

    if(volume < lowest_volume) lowest_volume = volume;

    if(iaxc_silence_threshold > 0)
	return silent;
    else
	return volume < iaxc_silence_threshold;
}

void iaxc_calculate_output_levels(void *audio, int len) {
    calculate_level(audio, len, &output_level);
}

static int output_postprocess(void *audio, int len)
{
    iaxc_calculate_output_levels(audio, len);

    do_level_callback();

    return 0;
}

static struct iaxc_audio_codec *create_codec(int format) {
    switch (format) {
	case IAXC_FORMAT_GSM:
	  return iaxc_audio_codec_gsm_new();
	break;
	case IAXC_FORMAT_ULAW:
	  return iaxc_audio_codec_ulaw_new();
	break;
	case IAXC_FORMAT_ALAW:
	  return iaxc_audio_codec_alaw_new();
	break;
	case IAXC_FORMAT_SPEEX:
	  return iaxc_audio_codec_speex_new(&speex_settings);
	break;
#ifdef CODEC_ILBC
	case IAXC_FORMAT_ILBC:
	  return iaxc_audio_codec_ilbc_new();
	break;
#endif
	default:
	  /* ERROR: codec not supported */
	  fprintf(stderr, "ERROR: Codec not supported: %d\n", format);
	  return NULL;
    }
}

EXPORT void iaxc_set_speex_settings(int decode_enhance, float quality, int bitrate, int vbr, int abr, int complexity) {
  speex_settings.decode_enhance = decode_enhance;
  speex_settings.quality = quality;
  speex_settings.bitrate = bitrate;
  speex_settings.vbr = vbr;
  speex_settings.abr = abr;
  speex_settings.complexity = complexity;
}

int send_encoded_audio(struct iaxc_call *call, void *data, int format, int samples)
{
	unsigned char outbuf[1024];
	int outsize = 1024;
	int silent;
	int insize = samples;

	//fprintf(stderr, "in encode_audio, format=%d\n", format);

	/* update last input timestamp */
	gettimeofday( &timeLastInput, NULL ) ;

	silent = iaxc_input_postprocess(data,insize,8000);	

	if(silent) { 
	  if(!call->tx_silent) {  /* send a Comfort Noise Frame */
	    call->tx_silent = 1;
	    if(iaxc_filters & IAXC_FILTER_CN)
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
	if(format == 0)
	  return 0;

	/* create encoder if necessary */
	if(!call->encoder) {
	    call->encoder = create_codec(format);
	}

	if(!call->encoder) {
		  /* ERROR: no codec */
		  fprintf(stderr, "ERROR: Codec could not be created: %d\n", format);
		  return 0;
	}

	if(call->encoder->encode(call->encoder, &insize, (short *)data, &outsize, outbuf)) {
		  /* ERROR: codec error */
		  fprintf(stderr, "ERROR: encode error: %d\n", format);
		  return 0;
	}
	
	if(samples-insize == 0)
	{
	    fprintf(stderr, "ERROR encoding (no samples output (samples=%d)\n", samples);
	    return -1;
	}

	if(iax_send_voice(call->session,format, outbuf, 1024-outsize, samples-insize) == -1) 
	{
	      puts("Failed to send voice!");
	      return -1;
	}
	
	return 0;
}

/* decode encoded audio; return the number of bytes decoded 
 * negative indicates error */
int decode_audio(struct iaxc_call *call, void *out, void *data, int len, int format, int *samples)
{
	int insize = len;
	int outsize = *samples;
    
	//fprintf(stderr, "in decode_audio, format=%d\n", format);
	/* update last output timestamp */
	gettimeofday( &timeLastOutput, NULL ) ;

	//if(len == 0) fprintf(stderr, "Interpolation voice frame\n");

	if(format == 0) {
		fprintf(stderr, "decode_audio: Format is zero (should't happen)!\n");
		return -1;
	}

	/* destroy decoder if it is incorrect type */
	if(call->decoder && call->decoder->format != format)
	{
	    call->decoder->destroy(call->decoder);
	    call->decoder = NULL;
	}

	/* create encoder if necessary */
	if(!call->decoder) {
	    call->decoder = create_codec(format);
	}

	if(!call->decoder) {
		  /* ERROR: no codec */
		  fprintf(stderr, "ERROR: Codec could not be created: %d\n", format);
		  return -1;
	}

	if(call->decoder->decode(call->decoder, &insize, data, &outsize, out)) {
		  /* ERROR: codec error */
		  fprintf(stderr, "ERROR: decode error: %d\n", format);
		  return -1;
	}

	output_postprocess(out, *samples-outsize);	

	*samples = outsize;
	return len-insize;
}

