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

#include "codec_gsm.h"
#include "iaxclient_lib.h"

struct state {
    gsm gsmstate;
    plc_state_t plc;
};


static void destroy ( struct iaxc_audio_codec *c) {

    struct state * encstate = (struct state *) c->encstate;
    struct state * decstate = (struct state *) c->decstate;

    gsm_destroy(encstate->gsmstate);
    gsm_destroy(decstate->gsmstate);
    free(c->encstate);
    free(c->decstate);
    free(c);
}


static int decode ( struct iaxc_audio_codec *c, 
    int *inlen, unsigned char *in, int *outlen, short *out ) {
    struct state * decstate = (struct state *) c->decstate;

    /* use generic interpolation */
    if(*inlen == 0) {
        int interp_len = 160;
        if(*outlen < interp_len) interp_len = *outlen;
        plc_fillin(&decstate->plc,out,interp_len);
        *outlen -= interp_len;
        return 0;
    }

    /* need to decode minimum of 33 bytes to 160 byte output */
    while( (*inlen >= 33) && (*outlen >= 160) ) {
	if(gsm_decode(decstate->gsmstate, in, out))
	{
	    fprintf(stderr, "codec_gsm: gsm_decode returned error\n");
	    return -1;
	}

	/* push decoded data to interpolation buffer */
	plc_rx(&decstate->plc,out,160);

	/* we used 33 bytes of input, and 160 bytes of output */
	*inlen -= 33; 
	in += 33;
	*outlen -= 160;
	out += 160;
    }

    return 0;
}

static int encode ( struct iaxc_audio_codec *c, 
    int *inlen, short *in, int *outlen, unsigned char *out ) {

    struct state * encstate = (struct state *) c->encstate;


    /* need to encode minimum of 160 bytes to 33 byte output */
    while( (*inlen >= 160) && (*outlen >= 33) ) {
	gsm_encode(encstate->gsmstate, in, out);

	/* we used 160 bytes of input, and 33 bytes of output */
	*inlen -= 160; 
	in += 160;
	*outlen -= 33;
	out += 33;
    }

    return 0;
}

struct iaxc_audio_codec *iaxc_audio_codec_gsm_new() {
  
  struct state * encstate;
  struct state * decstate;
  struct iaxc_audio_codec *c = calloc(sizeof(struct iaxc_audio_codec),1);

  
  if(!c) return c;

  strcpy(c->name,"gsm 06.10");
  c->format = IAXC_FORMAT_GSM;
  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  c->minimum_frame_size = 160;

  c->encstate = calloc(sizeof(struct state),1);
  c->decstate = calloc(sizeof(struct state),1);

  /* leaks a bit on no-memory */
  if(!(c->encstate && c->decstate)) 
      return NULL;

  encstate = (struct state *) c->encstate;
  decstate = (struct state *) c->decstate;

  encstate->gsmstate = gsm_create();
  decstate->gsmstate = gsm_create();

  if(!(encstate->gsmstate && decstate->gsmstate)) 
      return NULL;

  return c;
}

