/*
 * iaxclient: a portable telephony toolkit
 *
 * Copyright (C) 2003-2005, Horizon Wimba, Inc.
 *
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 * A video codec using the theora library.
 */

#include "iaxclient_lib.h"
#include "codec_theora.h"
#include <theora/theora.h>
#include <math.h>

static int initialized;

typedef struct {
    theora_state td;
    theora_info	 ti;
    theora_comment tc;
    int setup_done;
    uint32_t crc;
} mytheora;

static void destroy ( struct iaxc_video_codec *c) {
    free(c);
}

static unsigned int crc32 (int length, unsigned char *crcdata)
{
   int index, loop;
   unsigned int byte, crc, mask;

   index = 0;
   crc = 0xFFFFFFFF;

   while (index < length) {
       byte = crcdata [index];
       crc = crc ^ byte;

       for (loop = 7; loop >= 0; loop--) {
	   mask = -(crc & 1);
	   crc = (crc >> 1) ^ (0xEDB88320 & mask);
       }
       index++;
   }
   return ~crc;
}


static int decode ( struct iaxc_video_codec *c, 
    int *inlen, char *in, int *outlen, char *out ) {
    int ret;
    uint32_t crc;  
    int count;
    mytheora *d = (mytheora *)c->decstate;

    //fprintf(stderr, "codec_theora: inlen = %d\n", *inlen);

    if(*inlen < 8) {
	fprintf(stderr, "midget theora frame, dropping\n");
	return -1;
    }

    /* read the CRC */
    crc = *(uint32_t *)in;
    if(crc != d->crc) {
      d->setup_done = 0;
    }
    in+=4; (*inlen)-=4;

    /* read the count; ignore frag/cont for now */
    count = (*(unsigned char *)in) & 0x0f;
    in++; (*inlen)--;

    //fprintf(stderr, "codec_theora: got %d sub-packets to decode\n", count);

    while(count--) {
	uint8_t type;
	uint16_t len; 
	ogg_packet op;

	if(*inlen < 3) {
	    fprintf(stderr, "codec_theora: truncated theora frame, dropping (need 3 bytes for type, len, got %d\n", *inlen);
	    return -1;
	}

	type = (*(uint8_t *)in);
	in++; (*inlen)--;

	len = (*(uint16_t *)in);
	in+=2; (*inlen)-=2;

    //fprintf(stderr, "codec_theora: handling type %x, len %d\n", type, len);

	/* decode into an OP structure */
	memset(&op,0,sizeof(op));
	op.bytes = len;
	op.packet = in;

	in += len; (*inlen) -= len; 


	if(type & 0x80) {
	    if(type == 0x80) op.b_o_s = 1;
	    if((ret=theora_decode_header(&d->ti, &d->tc, &op))) {
		fprintf(stderr, "codec_theora: couldn't decode header type %x: %d\n", type, ret);
		return -1;
	    }
	    if(type == 0x82) {
      fprintf(stderr, "codec_theora: initializing decoder from setup packet\n", type, len);
		if(theora_decode_init(&d->td, &d->ti)) {
		    fprintf(stderr, "codec_theora: couldn't init from header type %x\n", type);
		    return -1;
		}
		d->crc = crc32(op.bytes,op.packet);
	    }

	} else if(type == 0x00) {
	    yuv_buffer picture;

	    if(d->crc != crc) {
		fprintf(stderr, "codec_theora: warning: not decoding frame, setup CRC mismatch: %x != %x\n", crc, d->crc);
		return -1;
	    }

	    if(theora_decode_packetin(&d->td,&op) == OC_BADPACKET) { 
		fprintf(stderr, "codec_theora: warning: theora_decode_packetin says bad packet\n");
		return -1;
	    }
	   
	    /* make sure we have enough room for the goodies */
	    if(*outlen < d->ti.width * d->ti.height * 6/4) {
		fprintf(stderr, "codec_theora: not enough room for decoding\n");
		return -1;
	    }

#if 0 // theora returns in it's own buffer 
	    picture.y = out;
	    picture.u = out + d->ti.width * d->ti.height;
	    picture.v = out + d->ti.width * d->ti.height * 5 / 4;

	    picture.y_width =  d->ti.width;
	    picture.y_height =  d->ti.height;
	    picture.y_stride =  d->ti.width;

	    picture.uv_width =  d->ti.width/2;
	    picture.uv_height =  d->ti.height/2;
	    picture.uv_stride =  d->ti.width/2;
#endif

	    /* finally, here's where we get our goodies */
	    if(theora_decode_YUVout(&d->td, &picture)) {
		fprintf(stderr, "codec_theora: error getting our goodies\n");
		return -1;
	    }
#if 1 // theora decodes in it's own buffer
	    {
		int line;

		//clear output
		memset(out,127,d->ti.height*d->ti.width*6/4);

		for(line = 0; line < d->ti.height/2; line++) {
		    // Y-even
		    memcpy(out + picture.y_width * 2*line    , picture.y + 2*line      * picture.y_stride, picture.y_width);
		    // Y-odd
		    memcpy(out + picture.y_width * (2*line+1), picture.y + (2*line+1)  * picture.y_stride, picture.y_width);

		    // U + V
		    memcpy(out + (d->ti.width * d->ti.height)       + line * d->ti.width/2, picture.u + line * picture.uv_stride, picture.uv_width);
		    memcpy(out + (d->ti.width * d->ti.height * 5/4) + line * d->ti.width/2, picture.v + line * picture.uv_stride, picture.uv_width);
		}
	    }
#endif
	    (*outlen) -= d->ti.width * d->ti.height * 6/4;
	    out += d->ti.width * d->ti.height * 6/4;

	    //fprintf(stderr, "codec_theora: decoded\n");

	} else {
	    fprintf(stderr, "codec_theora: unknown sub-packet type: %d, ignoring\n", type);
	}

    }
    
    return 0;
}


static int sz_output(void *data, int len, int *outsz, char **out) {
    if(len > *outsz) {
	fprintf(stderr, "codec_theora: output buffer full\n");
	return -1;
    }
    memcpy(*out,data,len);
    *outsz -= len;
    (*out) += len;
    return 0;
}

static int ogg_output(ogg_packet *op, unsigned char type, int *outsz, char **out) {
    int ret;
    uint16_t len;

    ret = sz_output(&type,1,outsz,out);
    if(ret) return ret;

    if(op->bytes > 65535) {
	fprintf(stderr, "codec_theora: ogg packet bigger than 65536?\n");
	return -1;
    }

    len = op->bytes;
    ret = sz_output(&len,2,outsz,out);
    if(ret) return ret;

    //fprintf(stderr, "ogg_output: type %x, len %d\n", type, len);

    return sz_output(op->packet, op->bytes, outsz, out);
}

static int send_setup_headers(struct iaxc_video_codec *c, int *inlen, char *in, int *outlen, char **out ) {
    mytheora *e = (mytheora *)c->encstate;
    ogg_packet op;

    /* create a packet with the three headers */

    /* first, we re-calculate the crc */
    theora_encode_tables(&e->td,&op);
    e->crc = crc32(op.bytes,op.packet);

    /* now, write the CRC */
    sz_output((char *)&e->crc,4,outlen,out);

    /* 4 packets will follow */
    sz_output("\004",1,outlen,out);
    
  
    /* ident header */
    theora_encode_header(&e->td,&op);
    ogg_output(&op,0x80,outlen,out);

    /* comment header */
    theora_comment_init(&e->tc);
    theora_encode_comment(&e->tc,&op);
    ogg_output(&op,0x81,outlen,out);


    /* tables/setup header */
    theora_encode_tables(&e->td,&op);
    ogg_output(&op,0x82,outlen,out);

    return 0;
}


static int encode ( struct iaxc_video_codec *c, 
    int *inlen, char *in, int *outlen, char *out ) {
    int ret;
    mytheora *e = (mytheora *)c->encstate;
    ogg_packet op;
    yuv_buffer picture;
    int outsz;

    
    if(!e->setup_done) {
	if(send_setup_headers(c, inlen, in, outlen, &out )) {
	    fprintf(stderr, "codec_theora: error sending setup headers\n");
	    return -1;
	}
	e->setup_done = 1;	
    } else {
	/* send the header */
	/* write the CRC */
	sz_output((char *)&e->crc,4,outlen,&out);

	/* 1 packet will follow */
	sz_output("\001",1,outlen,&out);
    }

    /* set up the picture buffer */ 
    picture.y = in;
    picture.u = in + e->ti.width * e->ti.height;
    picture.v = in + e->ti.width * e->ti.height * 5 / 4;

    picture.y_width =  e->ti.width;
    picture.y_height =  e->ti.height;
    picture.y_stride =  e->ti.width;

    picture.uv_width =  e->ti.width/2;
    picture.uv_height =  e->ti.height/2;
    picture.uv_stride =  e->ti.width/2;

    /* encode */
    ret = theora_encode_YUVin(&e->td, &picture);
    if(ret) {
	fprintf(stderr, "codec_theora: error in theora_encode_YUVin: %d\n", ret);
	return -1;
    }

    /* get the data */
    ret = theora_encode_packetout(&e->td, 0, &op);
    if(ret != 1) {
	fprintf(stderr, "codec_theora: error in theora_encode_packetout: %d\n", ret);
	return -1;
    }

    /* send it */
    return ogg_output(&op,0x00,outlen,&out);
}


struct iaxc_video_codec *iaxc_video_codec_theora_new(int format, int w, int h, int framerate, int bitrate) {
  mytheora *e, *d;
  char *name;

  struct iaxc_video_codec *c = calloc(sizeof(struct iaxc_video_codec),1);
  if(!c) return NULL;
  
  c->decstate = calloc(sizeof(mytheora),1);
  if(!c->decstate) return NULL;

  c->encstate = calloc(sizeof(mytheora),1);
  if(!c->encstate) return NULL;
  
  c->format = format;

  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  e = (mytheora *)c->encstate;
  d = (mytheora *)c->decstate;


  /* set up some parameters in the contexts */

  theora_info_init(&e->ti);

  /* set up common parameters */
  e->ti.width = w;
  e->ti.height = h;
  e->ti.frame_width = w;
  e->ti.frame_height = h;

  e->ti.fps_numerator = framerate;
  e->ti.fps_denominator = 1;

  e->ti.aspect_numerator = 1;
  e->ti.aspect_denominator = 1;

  e->ti.colorspace = OC_CS_UNSPECIFIED;
  //e->ti.pixelformat = OC_PF_420;

  e->ti.target_bitrate = bitrate;

  e->ti.quality = 0;

  e->ti.dropframes_p=0;
  e->ti.quick_p=1;
  e->ti.keyframe_auto_p=0;
  e->ti.keyframe_frequency=framerate;
  e->ti.keyframe_frequency_force=framerate;
  e->ti.keyframe_data_target_bitrate=bitrate;
  e->ti.keyframe_auto_threshold=80;
  e->ti.keyframe_mindistance=8;
  e->ti.noise_sensitivity=0;

  theora_encode_init(&e->td,&e->ti);
  
  /* decoder */
  theora_info_init(&d->ti);
  theora_comment_init(&d->tc);

  strcpy(c->name,"Theora");

  return c;
}

