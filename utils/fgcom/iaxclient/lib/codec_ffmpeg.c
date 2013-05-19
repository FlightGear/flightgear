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
 * A video codec using the ffmpeg library.
 */

#include "iaxclient_lib.h"
#include "codec_ffmpeg.h"
#include <ffmpeg/avcodec.h>
#include <math.h>

static int initialized;

static void destroy ( struct iaxc_video_codec *c) {
    free(c);
}


static int decode ( struct iaxc_video_codec *c, 
    int *inlen, char *in, int *outlen, char *out ) {
    AVCodecContext *d = (AVCodecContext *)c->decstate;
    AVFrame *frame;
    int got_picture;

    frame = avcodec_alloc_frame();

    if(avcodec_decode_video(d, frame, &got_picture, in, *inlen) < 0) {
	fprintf(stderr, "codec_ffmpeg: error decoding picture\n");
	return -1;
    }

    if(!got_picture) {
	fprintf(stderr, "codec_ffmpeg: no picture decoding picture?\n");
	return -1;
    }

    //fprintf(stderr, "codec_ffmpeg: copying decoded data, %d bytes\n", d->width * d->height * 6/4);

#if 0
    memcpy(out, frame->data[0], d->width * d->height);
    memcpy(out + d->width * d->height, frame->data[1], d->width * d->height / 4);
    memcpy(out + d->width * d->height * 5 / 4, frame->data[2], d->width * d->height / 4);
#else
    {
	int line;

	//clear output
	memset(out,127,d->height*d->width*6/4);

	for(line = 0; line < d->height/2; line++) {
	    // Y-even
	    memcpy(out + d->width * 2*line      , frame->data[0] + 2*line     * frame->linesize[0], d->width);
	    // Y-odd
	    memcpy(out + d->width * (2*line + 1), frame->data[0] + (2*line+1) * frame->linesize[0], d->width);

	    // U + V
	    memcpy(out + (d->width * d->height) + line * d->width/2, frame->data[1] + line     * frame->linesize[1], d->width/2);
	    memcpy(out + (d->width * d->height * 5/4) + line * d->width/2, frame->data[2] + line     * frame->linesize[2], d->width/2);
	    
	}
    }
#endif


    *outlen -= d->width * d->height * 6/4;
    av_free(frame);

    return 0;
}

static double psnr(double d){
    if(d==0) return HUGE_VAL;
    return -10.0*log(d)/log(10.0);
}

#define CONVERT_JPEG_CCIR

static int encode ( struct iaxc_video_codec *c, 
    int *inlen, char *in, int *outlen, char *out ) {
    AVCodecContext *e = (AVCodecContext *)c->encstate;
    AVFrame *frame;
    int outsz;

    frame = avcodec_alloc_frame();
    
    frame->data[0] = in;
    frame->data[1] = in + e->width * e->height;
    frame->data[2] = in + e->width * e->height * 5 / 4;
    frame->linesize[0] =  e->width;
    frame->linesize[1] =  e->width/2;
    frame->linesize[2] =  e->width/2;
    //frame->pts = e->frame_number * AV_TIME_BASE * e->frame_rate_base / e->frame_rate;
    frame->pts = AV_NOPTS_VALUE;
    //frame->key_frame = 1;

    //frame->quality = 3;
#ifdef CONVERT_JPEG_CCIR    
    {
	AVPicture inp, outp;
	inp.data[0] = in;
	inp.data[1] = in + e->width * e->height;
	inp.data[2] = in + e->width * e->height * 5 / 4;

	outp.data[0] = in;
	outp.data[1] = in + e->width * e->height;
	outp.data[1] = in + e->width * e->height * 5 / 4;

	outp.linesize[0] = inp.linesize[0] = e->width;
	outp.linesize[1] = inp.linesize[1] = e->width/2;
	outp.linesize[1] = inp.linesize[2] = e->width/2;

	//if(img_convert(&outp,PIX_FMT_YUV420P,&inp,PIX_FMT_YUVJ420P, e->width, e->height)){
	if(img_convert((AVPicture *)frame,PIX_FMT_YUV420P,(AVPicture *)frame,PIX_FMT_YUVJ420P, e->width, e->height)){
	  fprintf(stderr, "codec_ffmpeg:encode: can't convert colorspace\n");
	  return -1;
	}

	//frame->data[0] = convbuf;
	//frame->data[1] = convbuf + e->width * e->height;
	//frame->data[2] = convbuf + e->width * e->height * 5 / 4;

    }
#endif

#if 0
    /* zero chroma */
    {
      int i;
      for(i=0;i<e->width*e->height/2;i++) {
	in[(e->width * e->height) + i] = 127;
      }
    }
#endif
	

    outsz = avcodec_encode_video(e, out, *outlen, frame);
    if(!outsz)
    {
	fprintf(stderr, "codec_ffmpeg: encode failed\n");
	free(frame);
	return -1;
    }
    (*outlen) -= outsz;

    {
	char buf[2048];
	avcodec_string(buf,2048,e,1);
	fprintf(stderr, "codec_ffmpeg: %s coded %d bits, q=%2d psnr=%6.2f\r", buf, outsz*8, e->coded_frame->quality, 
		psnr(e->coded_frame->error[0]/(e->width*e->height*255.0*255.0)));	
    }
    //do_video_stats(e, outsz);

    free(frame);
    return 0;
}


struct iaxc_video_codec *iaxc_video_codec_ffmpeg_new(int format, int w, int h, int framerate, int bitrate) {
  AVCodecContext *e;
  AVCodecContext *d;
  AVCodec *codec;
  int ff_enc_id, ff_dec_id;
  char *name;

  struct iaxc_video_codec *c = calloc(sizeof(struct iaxc_video_codec),1);
  if(!c) return NULL;
  
  c->decstate = calloc(sizeof(AVCodecContext *),1);
  if(!c->decstate) return NULL;

  c->encstate = calloc(sizeof(AVCodecContext *),1);
  if(!c->encstate) return NULL;
  
  if(!initialized) { 
      avcodec_register_all();
      initialized = 1; 
  }


  c->format = format;

  c->encode = encode;
  c->decode = decode;
  c->destroy = destroy;

  /* Get contexts for them */
  c->encstate = avcodec_alloc_context();
  if(!c->encstate) return NULL;
  e = c->encstate;

  c->decstate = avcodec_alloc_context();
  if(!c->decstate) return NULL;
  d = c->decstate;

  /* set up some parameters in the contexts */

  e->frame_rate = framerate;
  e->frame_rate_base = 1;
  e->width = w;
  e->height = h;

  e->gop_size = framerate * 5;
  e->pix_fmt = PIX_FMT_YUV420P;

#if 0
  e->bit_rate_tolerance = bitrate * 20;
  e->rc_min_rate = 0;
  e->rc_max_rate = bitrate * 3/2;
  e->rc_buffer_size = 2048;
  e->rc_qsquish = 0;
  e->rc_eq = "tex^qComp";
  e->mb_qmin = e->qmin = 2;
  e->mb_qmax = e->qmax = 10;
  e->lmin = 2*FF_QP2LAMBDA;
  e->lmax = 10*FF_QP2LAMBDA;
  e->qcompress = 0.5;
  e->i_quant_offset = (float)0.0; // qscale offset between p and i frames
  e->max_qdiff = 3;
  e->flags |= CODEC_FLAG_QSCALE;
  e->global_quality = FF_QP2LAMBDA * 2;
#endif

  // maybe this will be worth changing later.
  e->i_quant_factor = (float)-0.6; // qscale factor between p and i frames  default 0.8

  // these are settings which are in ffmpeg, but different in avcodec_alloc_context defaults
  e->qblur = 0.5;
  // end ffmpeg differences.

//  e->lmin = 0.1*FF_QP2LAMBDA;
//  e->lmax = 10*FF_QP2LAMBDA;

  e->bit_rate = bitrate;
  e->flags |= CODEC_FLAG_PSNR;

  //e->flags |= CODEC_FLAG_QSCALE;
  //e->global_quality = 1;

  // debug ffmpeg ratecontrol
  e->debug |= FF_DEBUG_RC;

  //e->me_method = ME_EPZS;
  //e->mb_decision = FF_MB_DECISION_SIMPLE;
  ////e->me_subpel_quality = 8;

  switch(format) {
    case IAXC_FORMAT_H261:
	ff_enc_id = ff_dec_id = CODEC_ID_H261;
	name = "H.261";
    break;
    case IAXC_FORMAT_H263:
//	ff_enc_id = ff_dec_id = CODEC_ID_H263;
//	name = "H.263";
	ff_enc_id = ff_dec_id = CODEC_ID_SNOW;
	name = "Snow";
	e->strict_std_compliance = -1;
    break;
    default:
    case IAXC_FORMAT_H263_PLUS:
	ff_enc_id = CODEC_ID_H263P;
	ff_dec_id = CODEC_ID_H263;
	name = "H.263+";
    break;
    case IAXC_FORMAT_MPEG4:
	ff_enc_id = ff_dec_id = CODEC_ID_MPEG4;
  	e->flags |= CODEC_FLAG_LOW_DELAY;
	name = "MPEG4";
    break;
    case IAXC_FORMAT_H264:
	ff_enc_id = ff_dec_id = CODEC_ID_H264;
	name = "H.264";
    break;
    case IAXC_FORMAT_THEORA:
	ff_enc_id = ff_dec_id = CODEC_ID_THEORA;
	name = "Theora";
    break;
  }

   
  strcpy(c->name,"ffmpeg-");
  strncat(c->name, name, sizeof(c->name));

  /* Get the codecs */
  codec = avcodec_find_encoder(ff_enc_id);
  if(!codec) {
      fprintf(stderr, "codec_ffmpeg: can't find encoder %d\n", ff_enc_id);
      return NULL;
  }
  if(avcodec_open(e, codec)) {
      fprintf(stderr, "codec_ffmpeg: can't open encoder %s\n", name);
      return NULL;
  }

  codec = avcodec_find_decoder(ff_dec_id);
  if(!codec) {
      fprintf(stderr, "codec_ffmpeg: can't find decoder %d\n", ff_dec_id);
      return NULL;
  }
  if(avcodec_open(d, codec)) {
      fprintf(stderr, "codec_ffmpeg: can't open decoder %s\n", name);
      return NULL;
  }

  {
      enum PixelFormat fmts[] = { PIX_FMT_YUV420P, -1 };
      if(d->get_format(d, fmts) != PIX_FMT_YUV420P) {
	  fprintf(stderr, "codec_ffmpeg: can't set decode format to YUV420P\n");
	  return NULL;
      }
  }

  return c;
}

