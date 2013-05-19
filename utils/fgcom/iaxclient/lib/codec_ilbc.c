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

#include "codec_ilbc.h"
#include "iaxclient_lib.h"
#include "iLBC/iLBC_encode.h"
#include "iLBC/iLBC_decode.h"


static void destroy ( struct iaxc_audio_codec *c) {
    free(c->encstate);
    free(c->decstate);
    free(c);
}


static int decode ( struct iaxc_audio_codec *c, 
    int *inlen, char *in, int *outlen, short *out ) {

    float fbuf[240];
    int i;

    if(*inlen == 0) {
        //fprintf(stderr, "ILBC Interpolate\n");
	iLBC_decode(fbuf, NULL, c->decstate, 0);
	for(i=0;i<240;i++)
	    out[i] = fbuf[i];
        *outlen -= 240;
        return 0;
    }


    /* need to decode minimum of 33 bytes to 160 byte output */
    if( (*inlen < 50) || (*outlen < 240) ) {
	fprintf(stderr, "codec_ilbc: inlen = %d outlen= %d\n",*inlen,*outlen);
	return -1;
    }

    while( (*inlen >= 50) && (*outlen >= 240) ) {
	iLBC_decode(fbuf, in, c->decstate, 1);
	for(i=0;i<240;i++)
	    out[i] = fbuf[i];

	out += 240;
	*outlen -= 240;
	in += 50;
	*inlen -= 50;
    }

    return 0;
}

static int encode ( struct iaxc_audio_codec *c, 
    int *inlen, short *in, int *outlen, char *out ) {

    float fbuf[240];
    int i;

    while( (*inlen >= 240) && (*outlen >= 50) ) {

	for(i=0;i<240;i++)
	    fbuf[i] = in[i];

	iLBC_encode(out,fbuf, c->encstate);

	out += 50;
	*outlen -= 50;
	in += 240;
	*inlen -= 240;
    }

    return 0;
}

struct iaxc_audio_codec *iaxc_audio_codec_ilbc_new() {
  struct iaxc_audio_codec *c = calloc(sizeof(struct iaxc_audio_codec),1);

  
  if(!c) return c;

  strcpy(c->name,"iLBC");
  c->format = IAXC_FORMAT_ILBC;
  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  c->minimum_frame_size = 240;

  c->encstate = calloc(sizeof(iLBC_Enc_Inst_t),1);
  c->decstate = calloc(sizeof(iLBC_Dec_Inst_t),1);

  /* leaks a bit on no-memory */
  if(!(c->encstate && c->decstate)) 
      return NULL;

  /* the 30 parameters are used for the latest iLBC sources, in 
   * http://www.ietf.org/internet-drafts/draft-ietf-avt-ilbc-codec-05.txt 
   * as used in asterisk-CVS as of 14 Oct 2004 */
  initEncode(c->encstate, 30);
  initDecode(c->decstate, 30, 1); /* use enhancer */

  return c;
}

