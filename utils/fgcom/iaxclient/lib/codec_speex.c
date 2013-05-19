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

#include "codec_speex.h"
#include "iaxclient_lib.h"
#include "speex/speex.h"

/* see codec_speex.h for termination mode definition;  Zero is the "asterisk-compatible" method --
 * generally, any other choice will not be compatible with established practice */

struct state {
      void *state;
      int frame_size;
      int termination_mode;
      SpeexBits bits;
};


static void destroy ( struct iaxc_audio_codec *c) {
    struct state * encstate = (struct state *) c->encstate;
    struct state * decstate = (struct state *) c->decstate;

    speex_bits_destroy(&encstate->bits);
    speex_bits_destroy(&decstate->bits);
    speex_encoder_destroy(encstate->state);
    speex_decoder_destroy(decstate->state);

    free(c->encstate);
    free(c->decstate);

    free(c);
}


static int decode ( struct iaxc_audio_codec *c, 
    int *inlen, unsigned char *in, int *outlen, short *out ) {

    struct state * decstate = (struct state *) c->decstate;
    int ret =0;
    int bits_left = 0;
    //int start_bits = 0;

    if(*inlen == 0) {
	//return 0;
	//fprintf(stderr, "Speex Interpolate\n");
	speex_decode_int(decstate->state, NULL, out);
	*outlen -= decstate->frame_size;
	return 0;
    }

    /* XXX if the input contains more than we can read, we lose here */
    speex_bits_read_from(&decstate->bits, (char *) in, *inlen);
    *inlen = 0; 

    //start_bits = speex_bits_remaining(&decstate->bits);

    while(speex_bits_remaining(&decstate->bits) && (*outlen >= decstate->frame_size)) 
    {
        ret = speex_decode_int(decstate->state, &decstate->bits, out);
// * from speex/speex.h, speex_decode returns:
// * @return return status (0 for no error, -1 for end of stream, -2 other)
        if (ret == 0) {
            //fprintf(stderr, "decode: inlen=%d outlen=%d\n", *inlen, *outlen);
        /* one frame of output */
            *outlen -= decstate->frame_size;
            out += decstate->frame_size;
	    if(decstate->termination_mode == IAXC_SPEEX_TERMINATION_NONE) {
		bits_left = speex_bits_remaining(&decstate->bits) % 8;	
		if(bits_left)
		    speex_bits_advance(&decstate->bits, bits_left);
	    }
        } else if (ret == -1) {
	    /* at end of stream, or just a terminator */
            bits_left = speex_bits_remaining(&decstate->bits) % 8;
	    if(bits_left >= 5)
		speex_bits_advance(&decstate->bits, bits_left);
	    else
                break;
        } else {
	    /* maybe there's not a whole frame somehow? */
            fprintf(stderr, "decode_int returned non-zero => %d\n",ret);
	  break;
      }
    }
    return 0;
}

static int encode ( struct iaxc_audio_codec *c, 
    int *inlen, short *in, int *outlen, unsigned char *out ) {

    int bytes;
  //int bitrate;
    struct state * encstate = (struct state *) c->encstate;

    /* need to encode minimum of encstate->frame_size samples */

    if(encstate->termination_mode) {
	    /* abnormal termination modes: separate frames, with or without termination in-between */
	    while(*inlen >= encstate->frame_size) 
	    {
	        /* reset and encode*/
    	    speex_bits_reset(&encstate->bits);
    	    speex_encode_int(encstate->state, in, &encstate->bits);

	        /* add terminator */
    	    if(encstate->termination_mode != IAXC_SPEEX_TERMINATION_NONE)
		    speex_bits_pack(&encstate->bits, 15, 5);

    	    /* write to output */
    	    /* can an error happen here?  no bytes? */
    	    bytes = speex_bits_write(&encstate->bits, (char *) out, *outlen);
            //fprintf(stderr, "encode wrote %d bytes, outlen was %d\n", bytes, *outlen);
    	    /* advance pointers to input and output */
    	    *inlen -= encstate->frame_size;
    	    in += encstate->frame_size;
    	    *outlen -= bytes;
    	    out += bytes;
	    } 
    } else {

	/*  only add terminator at end of bits */
	speex_bits_reset(&encstate->bits);

	/* need to encode minimum of encstate->frame_size samples */
	while(*inlen >= encstate->frame_size) {
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
    }
    return 0;
}

struct iaxc_audio_codec *iaxc_audio_codec_speex_new(struct iaxc_speex_settings *set) {
  
  struct state * encstate;
  struct state * decstate;
  struct iaxc_audio_codec *c = calloc(sizeof(struct iaxc_audio_codec),1);
  const SpeexMode *sm;

  if(!c) return c;

  strcpy(c->name,"speex");
  c->format = IAXC_FORMAT_SPEEX;
  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  c->encstate = calloc(sizeof(struct state),1);
  c->decstate = calloc(sizeof(struct state),1);

  /* leaks a bit on no-memory */
  if(!(c->encstate && c->decstate)) 
      return NULL;

  encstate = (struct state *) c->encstate;
  decstate = (struct state *) c->decstate;

  sm = set->mode;
  if(!sm) sm = &speex_nb_mode;


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
      int quality = set->quality;
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
  speex_encoder_ctl(decstate->state,SPEEX_GET_FRAME_SIZE,&decstate->frame_size);

  /* XXX: for some reason, with narrowband, we get 0 for decstate sometimes? */
  if(!decstate->frame_size) decstate->frame_size = 160;
  if(!encstate->frame_size) encstate->frame_size = 160;

  c->minimum_frame_size = 160;

  if(encstate->frame_size >  c->minimum_frame_size)  c->minimum_frame_size = encstate->frame_size;
  if(decstate->frame_size >  c->minimum_frame_size)  c->minimum_frame_size = decstate->frame_size;

  encstate->termination_mode = set->termination_mode;
  decstate->termination_mode = set->termination_mode;

  if(!(encstate->state && decstate->state)) 
      return NULL;

  return c;
}

