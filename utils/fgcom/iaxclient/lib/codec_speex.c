/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2004, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#include "codec_speex.h"
#include "iaxclient_lib.h"
#include "speex/speex.h"

struct State
{
	void *state;
	int frame_size;
	SpeexBits bits;
};


static void destroy ( struct iaxc_audio_codec *c)
{
	struct State * encstate = (struct State *) c->encstate;
	struct State * decstate = (struct State *) c->decstate;

	speex_bits_destroy(&encstate->bits);
	speex_bits_destroy(&decstate->bits);
	speex_encoder_destroy(encstate->state);
	speex_decoder_destroy(decstate->state);

	free(c->encstate);
	free(c->decstate);

	free(c);
}


static int decode( struct iaxc_audio_codec *c,
		int *inlen, unsigned char *in, int *outlen, short *out )
{
	struct State * decstate = (struct State *) c->decstate;

	if ( *inlen == 0 )
	{
		speex_decode_int(decstate->state, NULL, out);
		*outlen -= decstate->frame_size;
		return 0;
	}

	speex_bits_read_from(&decstate->bits, (char *) in, *inlen);
	*inlen = 0;

	while ( speex_bits_remaining(&decstate->bits) &&
			*outlen >= decstate->frame_size )
	{
		int ret = speex_decode_int(decstate->state, &decstate->bits, out);

		// from speex/speex.h, speex_decode returns:
		// @return return status (0 for no error, -1 for end of stream, -2 other)
		if (ret == 0)
		{
			/* one frame of output */
			*outlen -= decstate->frame_size;
			out += decstate->frame_size;
		} else if (ret == -1)
		{
			/* at end of stream, or just a terminator */
			int bits_left = speex_bits_remaining(&decstate->bits) % 8;
			if(bits_left >= 5)
				speex_bits_advance(&decstate->bits, bits_left);
			else
				break;
		} else
		{
			/* maybe there's not a whole frame somehow? */
			fprintf(stderr, "decode_int returned non-zero => %d\n",ret);
			break;
		}
	}
	return 0;
}

static int encode( struct iaxc_audio_codec *c,
		int *inlen, short *in, int *outlen, unsigned char *out )
{
	int bytes;
	struct State * encstate = (struct State *) c->encstate;

	/* need to encode minimum of encstate->frame_size samples */

	/*  only add terminator at end of bits */
	speex_bits_reset(&encstate->bits);

	/* need to encode minimum of encstate->frame_size samples */
	while(*inlen >= encstate->frame_size)
	{
		//fprintf(stderr, "encode: inlen=%d outlen=%d\n", *inlen, *outlen);
		speex_encode_int(encstate->state, in, &encstate->bits);
		*inlen -= encstate->frame_size;
		in += encstate->frame_size;
	}

	/* add terminator */
	speex_bits_pack(&encstate->bits, 15, 5);

	bytes = speex_bits_write(&encstate->bits, (char *) out, *outlen);

	/* can an error happen here?  no bytes? */
	*outlen -= bytes;

	return 0;
}

struct iaxc_audio_codec *codec_audio_speex_new(struct iaxc_speex_settings *set)
{
	struct State * encstate;
	struct State * decstate;
	struct iaxc_audio_codec *c = (struct iaxc_audio_codec *)calloc(sizeof(struct iaxc_audio_codec),1);
	const SpeexMode *sm;

	if(!c)
		return c;

	strcpy(c->name,"speex");
	c->format = IAXC_FORMAT_SPEEX;
	c->encode = encode;
	c->decode = decode;
	c->destroy = destroy;

	c->encstate = calloc(sizeof(struct State),1);
	c->decstate = calloc(sizeof(struct State),1);

	/* leaks a bit on no-memory */
	if(!(c->encstate && c->decstate))
		return NULL;

	encstate = (struct State *) c->encstate;
	decstate = (struct State *) c->decstate;

	sm = speex_lib_get_mode(SPEEX_MODEID_NB);

	encstate->state = speex_encoder_init(sm);
	decstate->state = speex_decoder_init(sm);
	speex_bits_init(&encstate->bits);
	speex_bits_init(&decstate->bits);
	speex_bits_reset(&encstate->bits);
	speex_bits_reset(&decstate->bits);

	speex_decoder_ctl(decstate->state, SPEEX_SET_ENH, &set->decode_enhance);

	speex_encoder_ctl(encstate->state, SPEEX_SET_COMPLEXITY, &set->complexity);

	if(set->quality >= 0) {
		if(set->vbr) {
			speex_encoder_ctl(encstate->state, SPEEX_SET_VBR_QUALITY, &set->quality);
		} else {
			int quality = (int)set->quality;
			speex_encoder_ctl(encstate->state, SPEEX_SET_QUALITY, &quality);
		}
	}
	if(set->bitrate >= 0)
		speex_encoder_ctl(encstate->state, SPEEX_SET_BITRATE, &set->bitrate);
	if(set->vbr)
		speex_encoder_ctl(encstate->state, SPEEX_SET_VBR, &set->vbr);
	if(set->abr)
		speex_encoder_ctl(encstate->state, SPEEX_SET_ABR, &set->abr);

	/* set up frame sizes (normally, this is 20ms worth) */
	speex_encoder_ctl(encstate->state,SPEEX_GET_FRAME_SIZE,&encstate->frame_size);
	speex_decoder_ctl(decstate->state,SPEEX_GET_FRAME_SIZE,&decstate->frame_size);

	c->minimum_frame_size = 160;

	if(encstate->frame_size > c->minimum_frame_size)
		c->minimum_frame_size = encstate->frame_size;
	if(decstate->frame_size > c->minimum_frame_size)
		c->minimum_frame_size = decstate->frame_size;

	if(!(encstate->state && decstate->state))
		return NULL;

	return c;
}

