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

#include "codec_ulaw.h"
#include "iaxclient_lib.h"

struct state {
    plc_state_t plc;
};

static short ulaw_2lin [256];
static unsigned char lin_2ulaw [16384];
static int initialized=0;

/* this looks similar to asterisk, but comes from public domain code by craig reese
   I've just followed asterisk's table sizes for lin_2u, and also too lazy to do binary arith to decide which
   iterations to skip -- this way we get the same result.. */
static void initialize() {
    int i;

    /* ulaw_2lin */
    for(i=0;i<256;i++) {
	  int b = ~i;
	  int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764};
	  int sign, exponent, mantissa, sample;

	  sign = (b & 0x80);
	  exponent = (b >> 4) & 0x07;
	  mantissa = b & 0x0F;
	  sample = exp_lut[exponent] + (mantissa << (exponent + 3));
	  if (sign != 0) sample = -sample;
	  ulaw_2lin[i] = sample;
    }

    /* lin_2ulaw */
    for(i=-32767;i<32768;i+=4) {
	int sample = i;
	int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                             4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
	int sign, exponent, mantissa;
	unsigned char ulawbyte;

	/* Get the sample into sign-magnitude. */
	sign = (sample >> 8) & 0x80;		/* set aside the sign */
	if (sign != 0) sample = -sample;		/* get magnitude */
	if (sample > 32635) sample = 32635;		/* clip the magnitude */

	/* Convert from 16 bit linear to ulaw. */
	sample = sample + 0x84;
	exponent = exp_lut[(sample >> 7) & 0xFF];
	mantissa = (sample >> (exponent + 3)) & 0x0F;
	ulawbyte = ~(sign | (exponent << 4) | mantissa);
	if (ulawbyte == 0) ulawbyte = 0x02;	/* optional CCITT trap */

	lin_2ulaw[((unsigned short)i) >> 2] = ulawbyte;
    }

    initialized = 1;
}

static void destroy ( struct iaxc_audio_codec *c) {
	if ( c->decstate )
		free(c->decstate);
	free(c);
}


static int decode ( struct iaxc_audio_codec *c,
    int *inlen, unsigned char *in, int *outlen, short *out ) {
    struct state *state = (struct state *)c->decstate;
    short *orig_out = out;
    short sample;

    if(*inlen == 0) {
	int interp_len = 160;
	if(*outlen < interp_len) interp_len = *outlen;
	plc_fillin(&state->plc,out,interp_len);
	*outlen -= interp_len;
	return 0;
    }

    while ((*inlen > 0) && (*outlen > 0)) {
	sample = ulaw_2lin[(unsigned char)*(in++)];
	*(out++) = sample;
	(*inlen)--; (*outlen)--;
    }
    plc_rx(&state->plc, orig_out, (int)(out - orig_out));

    return 0;
}

static int encode ( struct iaxc_audio_codec *c,
    int *inlen, short *in, int *outlen, unsigned char *out ) {

    while ((*inlen > 0) && (*outlen > 0)) {
	*(out++) = lin_2ulaw[((unsigned short)*(in++)) >> 2];
	(*inlen)--; (*outlen)--;
    }

    return 0;
}


struct iaxc_audio_codec *codec_audio_ulaw_new() {

  struct iaxc_audio_codec *c = (struct iaxc_audio_codec *)calloc(sizeof(struct iaxc_audio_codec),1);

  if(!c) return c;

  if(!initialized) initialize();

  strcpy(c->name,"ulaw");
  c->format = IAXC_FORMAT_ULAW;
  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  /* really, we can use less, but don't want to */
  c->minimum_frame_size = 160;

  /* decoder state, used for interpolation */
  c->decstate = calloc(sizeof(struct state),1);
  plc_init(&((struct state *)c->decstate)->plc);

  return c;
}

