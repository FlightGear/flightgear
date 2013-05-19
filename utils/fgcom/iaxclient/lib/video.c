/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2005 Horizon Wimba, inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

#include "iaxclient_lib.h"
#include "codec_ffmpeg.h"
#include "codec_theora.h"

#define VIDEO_BUFSIZ (1<<19)

static struct iaxc_video_driver video;
static int iaxc_video_width = 352;
static int iaxc_video_height = 288;
//static int iaxc_video_width = 176;
//static int iaxc_video_height = 144;
static int iaxc_video_framerate = 15;
static int iaxc_video_bitrate = 90000;
int iaxc_video_format_preferred = IAXC_FORMAT_H263_PLUS;
static int iaxc_video_format_allowed = IAXC_FORMAT_H263_PLUS|IAXC_FORMAT_H263|IAXC_FORMAT_MPEG4|IAXC_FORMAT_H264|IAXC_FORMAT_THEORA;

// a fake call to hold encode/decode stuff when no call is in progress.
static struct iaxc_call idle_call;

int iaxc_video_mode = IAXC_VIDEO_MODE_NONE;

/* debug: check a yuv420p buffer to ensure it's within the CCIR range */
static int check_ccir_yuv(char *data) {
    int i;
    unsigned char pix;
    int err = 0;

    for(i=0;i<iaxc_video_width * iaxc_video_height; i++) {
	pix = *data++;
	if( (pix < 16) || pix > 235) {
	    fprintf(stderr, "check_ccir_yuv: Y pixel[%d] out of range: %d\n", i, pix);
	    err++;
	}
    }
    for(i=0;i< iaxc_video_width * iaxc_video_height / 2; i++) {
	pix = *data++;
	if( (pix < 16) || pix > 239) {
	    fprintf(stderr, "check_ccir_yuv: U/V pixel[%d] out of range: %d\n", i, pix);
	    err++;
	}
    }
    return err;
}  


EXPORT int iaxc_video_mode_set(int mode) {
  return iaxc_video_mode = mode;
}

EXPORT int iaxc_video_mode_get() {
  return iaxc_video_mode;
}

EXPORT void iaxc_video_format_set(int preferred, int allowed, int framerate, int bitrate, int width, int height) {
  iaxc_video_format_preferred = preferred;
  iaxc_video_format_allowed = allowed;
  iaxc_video_framerate = framerate;
  iaxc_video_bitrate = bitrate;
  iaxc_video_width = width;
  iaxc_video_height = height;
}



static struct iaxc_video_codec *create_codec(int format, int encode) {
    struct iaxc_video_codec *vc;
    // XXX fixme: format
    //return iaxc_video_codec_ffmpeg_new("h264",iaxc_video_width,iaxc_video_height,iaxc_video_framerate,iaxc_video_bitrate);  
    switch(format) {
    case IAXC_FORMAT_H261:
    case IAXC_FORMAT_H263:
    case IAXC_FORMAT_H263_PLUS:
    case IAXC_FORMAT_MPEG4:
#ifdef LINUX  /* XXX presently, I don't have modern ffmpeg built on OSX or
		 Win32 */
	return iaxc_video_codec_ffmpeg_new(format,iaxc_video_width,iaxc_video_height,iaxc_video_framerate,iaxc_video_bitrate);  
#else
	return NULL;
#endif
    break;
    case IAXC_FORMAT_H264:
#if 0
	if(encode)
	  return iaxc_video_codec_h264_new(format,iaxc_video_width,iaxc_video_height,iaxc_video_framerate,iaxc_video_bitrate);  
	else
#endif
#ifdef LINUX  /* XXX presently, I don't have modern ffmpeg built on OSX or
		 Win32 */
	return iaxc_video_codec_ffmpeg_new(format,iaxc_video_width,iaxc_video_height,iaxc_video_framerate,iaxc_video_bitrate);  
#else
	return NULL;
#endif
    break;
    case IAXC_FORMAT_THEORA:
	return iaxc_video_codec_theora_new(format,iaxc_video_width,iaxc_video_height,iaxc_video_framerate,iaxc_video_bitrate);  
    break;
    }
}


static void show_video_frame(void *videobuf) {
      iaxc_event e;
      e.type = IAXC_EVENT_VIDEO;
      // XXX e.ev.video.format = 
      e.ev.video.width = iaxc_video_width;
      e.ev.video.height = iaxc_video_height;
      e.ev.video.data = videobuf;
      iaxc_post_event(e);
}

/* try to get the next frame, encode and send */
int iaxc_send_video(struct iaxc_call *call)
{
	unsigned char *videobuf;
	char encoded_video[VIDEO_BUFSIZ/2];
	int encoded_video_len = sizeof(encoded_video);
	int format = iaxc_video_format_preferred;
     
	if(format == 0) {
		fprintf(stderr, "encode_video: Format is zero (should't happen)!\n");
		return -1;
	}

	if(!call) call = &idle_call;

	/* destroy vencoder if it is incorrect type */
	if(call->vencoder && call->vencoder->format != format)
	{
	    call->vencoder->destroy(call->vencoder);
	    call->vencoder = NULL;
	}

	/* create encoder if necessary */
	if(!call->vencoder) {
	    call->vencoder = create_codec(format, 1);
	}

	if(!call->vencoder) {
	    /* ERROR: no codec */
	    fprintf(stderr, "ERROR: Video codec could not be created: %d\n", format);
	    return -1;
	}

	// try to get the next frame
	video.start(&video);
	video.input(&video, &videobuf);
	if(!videobuf) {
	    fprintf(stderr, "iaxc_send_video: no frame\n");
	    return 0;
	}
	//fprintf(stderr, "Got frame\n");

	// debug.
	//if(check_ccir_yuv(videobuf)) {
	    //exit(1);
	//}


	// encode the frame
	{
	    int inlen, outlen;
	    inlen = iaxc_video_width * iaxc_video_height * 6 / 4;
	    outlen = encoded_video_len;
	    if(call->vencoder->encode(call->vencoder,&inlen,videobuf,&outlen,encoded_video)) {
		fprintf(stderr, "iaxc_send_video: encode failed\n");
		return -1;
	    }
	    encoded_video_len = encoded_video_len - outlen;
#if 1
	    {   // some stats here..
		fprintf(stderr, "iaxc_send_video: %d bits\n", encoded_video_len * 8);
	    }
#endif
	}

#if 0
	{ 
	  int i;
	  fprintf(stderr, "encoded video:");
	  for(i=0;i<32;i++) {
	      fprintf(stderr, " %02hhx", (unsigned char)encoded_video[i]);
	  }
	  fprintf(stderr, "\n");
	}
#endif

	if(iaxc_video_mode > IAXC_VIDEO_MODE_ACTIVE) {
	    if(iaxc_video_mode == IAXC_VIDEO_MODE_PREVIEW_RAW) {
		show_video_frame(videobuf);
	    } else if(iaxc_video_mode == IAXC_VIDEO_MODE_PREVIEW_ENCODED) {
		if(iaxc_receive_video(call, encoded_video, encoded_video_len, format, 1))
		{
		    fprintf(stderr, "iaxc_send_video: encoded preview decode failed\n");
		    return -1;
		}
	    }
	}

	// send the frame!
	
	return 0;
}

/* process an incoming video frame */
int iaxc_receive_video(struct iaxc_call *call, void *encoded_video, int encoded_video_len, int format, int preview)
{
	char videobuf[VIDEO_BUFSIZ];
	int outsize = VIDEO_BUFSIZ;
	int insize = encoded_video_len;

	if(!call) call = &idle_call;

	// if we're not in normal mode, and this isn't a preview-display frame, skip it.
	if(iaxc_video_mode != IAXC_VIDEO_MODE_ACTIVE && !preview ) return 0;

	if(format == 0) {
		fprintf(stderr, "decode_video: Format is zero (should't happen)!\n");
		return -1;
	}

	/* destroy vdecoder if it is incorrect type */
	if(call->vdecoder && call->vdecoder->format != format)
	{
	    call->vdecoder->destroy(call->vdecoder);
	    call->vdecoder = NULL;
	}

	/* create decoder if necessary */
	if(!call->vdecoder) {
	    call->vdecoder = create_codec(format, 0);
	}

	if(!call->vdecoder) {
		  /* ERROR: no codec */
		  fprintf(stderr, "ERROR: Video codec could not be created: %d\n", format);
		  return -1;
	}

	if(call->vdecoder->decode(call->vdecoder, &insize, encoded_video, &outsize, videobuf)) {
		  /* ERROR: codec error */
		  fprintf(stderr, "ERROR: decode error: %d\n", format);
		  return -1;
	}

	
	//fprintf(stderr, "video: decode succeeded, displaying\n");
	show_video_frame(videobuf);

	return 0;
}

int iaxc_video_initialize(void) {
       if(pv_initialize(&video, iaxc_video_width, iaxc_video_height, iaxc_video_framerate)) {
	     fprintf(stderr, "can't initialize pv\n");
	     return -1;
       }
       return 0;
}
		 
