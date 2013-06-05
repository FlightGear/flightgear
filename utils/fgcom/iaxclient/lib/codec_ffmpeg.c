/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Peter Grayson <jpgrayson@gmail.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 *
 * A video codec using the ffmpeg library.
 * 
 * TODO: this code still uses its own slicing mechanism
 * It should be converted to use the API provided in slice.[ch]
 */

#include <stdlib.h>

#include "codec_ffmpeg.h"
#include "iaxclient_lib.h"

#ifdef WIN32
#include "libavcodec/avcodec.h"
#else
#include <ffmpeg/avcodec.h>
#endif

struct slice_header_t
{
	unsigned char version;
	unsigned short source_id;
	unsigned char frame_index;
	unsigned char slice_index;
	unsigned char num_slices;
};

struct encoder_ctx
{
	AVCodecContext * avctx;
	AVFrame * picture;

	struct slice_header_t slice_header;

	unsigned char *frame_buf;
	int frame_buf_len;
};

struct decoder_ctx
{
	AVCodecContext * avctx;
	AVFrame * picture;

	struct slice_header_t slice_header;
	int frame_size;

	unsigned char * frame_buf;
	int frame_buf_len;
};

static struct slice_set_t * g_slice_set = 0;

static enum CodecID map_iaxc_codec_to_avcodec(int format)
{
	switch (format)
	{
	case IAXC_FORMAT_H261:
		return CODEC_ID_H261;

	case IAXC_FORMAT_H263:
		return CODEC_ID_H263;

	case IAXC_FORMAT_H263_PLUS:
		return CODEC_ID_H263P;

	case IAXC_FORMAT_MPEG4:
		return CODEC_ID_MPEG4;

	case IAXC_FORMAT_H264:
		return CODEC_ID_H264;

	case IAXC_FORMAT_THEORA:
		return CODEC_ID_THEORA;

	default:
		return CODEC_ID_NONE;
	}
}

static void destroy(struct iaxc_video_codec *c)
{
	if (c)
	{
		struct encoder_ctx *e = (struct encoder_ctx *) c->encstate;
		struct decoder_ctx *d = (struct decoder_ctx *) c->decstate;

		if (e)
		{
			av_freep(&e->avctx);
			av_freep(&e->picture);
			if (e->frame_buf)
				free(e->frame_buf);
			free(e);
		}

		if (d)
		{
			av_freep(&d->avctx);
			av_freep(&d->picture);
			if (d->frame_buf)
				free(d->frame_buf);
			free(d);
		}

		free(c);
	}
}

static void reset_decoder_frame_state(struct decoder_ctx * d)
{
	memset(d->frame_buf, 0, d->frame_buf_len);
	d->frame_size = 0;
	d->slice_header.slice_index = 0;
}

static int frame_to_frame_xlate(AVCodecContext * avctx, AVFrame * picture,
		int * outlen, char * out)
{
	int line;

	*outlen = avctx->width * avctx->height * 6 / 4;

	for ( line = 0; line < avctx->height / 2; ++line )
	{
		/* y even */
		memcpy(out + avctx->width * (2 * line + 0),
		       picture->data[0] + (2 * line + 0) * picture->linesize[0],
		       avctx->width);

		/* y odd */
		memcpy(out + avctx->width * (2 * line + 1),
		       picture->data[0] + (2 * line + 1) * picture->linesize[0],
		       avctx->width);

		/* u + v */
		memcpy(out + avctx->width * avctx->height
				+ line * avctx->width / 2,
		       picture->data[1] + line * picture->linesize[1],
		       avctx->width / 2);

		memcpy(out + avctx->width * avctx->height * 5 / 4
				+ line * avctx->width / 2,
		       picture->data[2] + line * picture->linesize[2],
		       avctx->width / 2);
	}

	return 0;
}

static int pass_frame_to_decoder(AVCodecContext * avctx, AVFrame * picture,
		int inlen, unsigned char * in, int * outlen, char * out)
{
	int bytes_decoded;
	int got_picture;

	bytes_decoded = avcodec_decode_video(avctx, picture, &got_picture,
			in, inlen);

	if ( bytes_decoded != inlen )
	{
		fprintf(stderr,
			"codec_ffmpeg: decode: failed to decode whole frame %d / %d\n",
			bytes_decoded, inlen);
		return -1;
	}

	if ( !got_picture )
	{
		fprintf(stderr,
			"codec_ffmpeg: decode: failed to get picture\n");
		return -1;
	}

	frame_to_frame_xlate(avctx, picture, outlen, out);

	return 0;
}

static char *parse_slice_header(char * in, struct slice_header_t * slice_header)
{
	slice_header->version     = in[0];
	slice_header->source_id   = (in[1] << 8) | in[2];
	slice_header->frame_index = in[3];
	slice_header->slice_index = in[4];
	slice_header->num_slices  = in[5];

	if ( slice_header->version != 0 )
	{
		fprintf(stderr,
			"codec_ffmpeg: decode: unknown slice header version %d\n",
			slice_header->version);
		return 0;
	}

	return in + 6;
}

static int decode_iaxc_slice(struct iaxc_video_codec * c, int inlen,
		char * in, int * outlen, char * out)
{
	struct decoder_ctx *d = (struct decoder_ctx *) c->decstate;
	struct slice_header_t * sh_saved = &d->slice_header;
	struct slice_header_t sh_this;
	char * inp;
	int ret;

	inp = parse_slice_header(in, &sh_this);

	if ( !inp )
		return -1;

	inlen -= inp - in;

	if ( sh_this.source_id == sh_saved->source_id )
	{
		unsigned char frame_delta;

		frame_delta = sh_this.frame_index - sh_saved->frame_index;

		if ( frame_delta > 20 )
		{
			/* This is an old slice. It's too late, we ignore it. */
			return 1;
		}
		else if ( frame_delta > 0 )
		{
			/* This slice belongs to a future frame */
			if ( sh_saved->slice_index > 0 )
			{
				/* We have received slices for a previous
				 * frame (e.g. the one we were previously
				 * working on), so we go ahead and send this
				 * partial frame to the decoder and get setup
				 * for the new frame.
				 */

				ret = pass_frame_to_decoder(d->avctx, d->picture,
						d->frame_size, d->frame_buf,
						outlen, out);

				reset_decoder_frame_state(d);

				if ( ret )
					return -1;
			}

			sh_saved->frame_index = sh_this.frame_index;
		}
	}
	else
	{
		sh_saved->source_id = sh_this.source_id;
		sh_saved->frame_index = sh_this.frame_index;
		sh_saved->slice_index = 0;
		d->frame_size = 0;
	}

	if ( c->fragsize * sh_this.slice_index + inlen > d->frame_buf_len )
	{
		fprintf(stderr,
			"codec_ffmpeg: decode: slice overflows decoder frame buffer\n");
		return -1;
	}

	memcpy(d->frame_buf + c->fragsize * sh_this.slice_index,
			inp, inlen);
	sh_saved->slice_index++;
	d->frame_size = c->fragsize * sh_this.slice_index + inlen;

	if ( sh_saved->slice_index < sh_this.num_slices )
	{
		/* Do not decode yet, there are more slices coming for
		 * this frame.
		 */
		return 1;
	}

	ret = pass_frame_to_decoder(d->avctx, d->picture, d->frame_size,
			d->frame_buf, outlen, out);

	reset_decoder_frame_state(d);

	if ( ret )
		return -1;

	return 0;
}

static int decode_rtp_slice(struct iaxc_video_codec * c,
		  int inlen, char * in, int * outlen, char * out)
{
	struct decoder_ctx *d = (struct decoder_ctx *) c->decstate;
	int ret = 1;

	while ( inlen )
	{
		int bytes_decoded;
		int got_picture;

		bytes_decoded = avcodec_decode_video(d->avctx, d->picture,
				&got_picture, (unsigned char *)in, inlen);

		if ( bytes_decoded < 0 )
		{
			fprintf(stderr,
				"codec_ffmpeg: decode: error decoding frame\n");
			return -1;
		}

		inlen -= bytes_decoded;
		in += bytes_decoded;

		if ( got_picture && ret == 0)
		{
			fprintf(stderr,
				"codec_ffmpeg: decode: unexpected second frame\n");
			return -1;
		}

		if ( got_picture )
		{
			frame_to_frame_xlate(d->avctx, d->picture, outlen, out);
			ret = 0;
		}
	}

	return ret;
}

static void slice_encoded_frame(struct slice_header_t * sh,
		struct slice_set_t * slice_set,
		unsigned char * in, int inlen, int fragsize)
{
	sh->num_slices = slice_set->num_slices = (inlen - 1) / fragsize + 1;

	for (sh->slice_index = 0; sh->slice_index < sh->num_slices;
			++sh->slice_index)
	{
		int slice_size = (sh->slice_index == sh->num_slices - 1) ?
			inlen % fragsize : fragsize;

		slice_set->size[sh->slice_index] = slice_size + 6;
		slice_set->data[sh->slice_index][0] = sh->version;
		slice_set->data[sh->slice_index][1] = sh->source_id >> 8;
		slice_set->data[sh->slice_index][2] = sh->source_id & 0xff;
		slice_set->data[sh->slice_index][3] = sh->frame_index;
		slice_set->data[sh->slice_index][4] = sh->slice_index;
		slice_set->data[sh->slice_index][5] = sh->num_slices;

		memcpy(&slice_set->data[sh->slice_index][6], in, slice_size);

		in += slice_size;
	}

	sh->frame_index++;
}

static int encode(struct iaxc_video_codec *c,
		int inlen, char * in, struct slice_set_t * slice_set)
{
	struct encoder_ctx *e = (struct encoder_ctx *) c->encstate;
	int encoded_size;

	avcodec_get_frame_defaults(e->picture);

	e->picture->data[0] = (unsigned char *)in;
	e->picture->data[1] = (unsigned char *)in
		+ e->avctx->width * e->avctx->height;
	e->picture->data[2] = (unsigned char *)in
		+ e->avctx->width * e->avctx->height * 5 / 4;

	e->picture->linesize[0] = e->avctx->width;
	e->picture->linesize[1] = e->avctx->width / 2;
	e->picture->linesize[2] = e->avctx->width / 2;

	/* TODO: investigate setting a real pts value */
	e->picture->pts = AV_NOPTS_VALUE;

	/* TODO: investigate quality */
	e->picture->quality = 10;

	g_slice_set = slice_set;
	slice_set->num_slices = 0;

	encoded_size = avcodec_encode_video(e->avctx,
			e->frame_buf, e->frame_buf_len, e->picture);

	if (!encoded_size)
	{
		fprintf(stderr, "codec_ffmpeg: encode failed\n");
		return -1;
	}

	slice_set->key_frame = e->avctx->coded_frame->key_frame;

	/* This is paranoia, of course. */
	g_slice_set = 0;

	/* We are in one of two modes here.
	 *
	 * The first possibility is that the codec supports rtp
	 * packetization. In this case, the slice_set has already been
	 * filled via encode_rtp_callback() calls made during the call
	 * to avcodec_encode_video().
	 *
	 * The second possibility is that we have one big encoded frame
	 * that we need to slice-up ourselves.
	 */

	if (!e->avctx->rtp_payload_size)
		slice_encoded_frame(&e->slice_header, slice_set,
				e->frame_buf, encoded_size, c->fragsize);

	return 0;
}

void encode_rtp_callback(struct AVCodecContext *avctx, void *data, int size,
		     int mb_nb)
{
	memcpy(&g_slice_set->data[g_slice_set->num_slices], data, size);
	g_slice_set->size[g_slice_set->num_slices] = size;
	g_slice_set->num_slices++;
}

struct iaxc_video_codec *codec_video_ffmpeg_new(int format, int w, int h,
						     int framerate, int bitrate,
						     int fragsize)
{
	struct encoder_ctx *e;
	struct decoder_ctx *d;
	AVCodec *codec;
	int ff_enc_id, ff_dec_id;
	char *name;

	struct iaxc_video_codec *c = calloc(sizeof(struct iaxc_video_codec), 1);

	if (!c)
	{
		fprintf(stderr,
			"codec_ffmpeg: failed to allocate video context\n");
		return NULL;
	}

	avcodec_init();
	avcodec_register_all();

	c->format = format;
	c->width = w;
	c->height = h;
	c->framerate = framerate;
	c->bitrate = bitrate;
	/* TODO: Is a fragsize of zero valid? If so, there's a divide
	 * by zero error to contend with.
	 */
	c->fragsize = fragsize;

	c->encode = encode;
	c->decode = decode_iaxc_slice;
	c->destroy = destroy;

	c->encstate = calloc(sizeof(struct encoder_ctx), 1);
	if (!c->encstate)
		goto bail;
	e = c->encstate;
	e->avctx = avcodec_alloc_context();
	if (!e->avctx)
		goto bail;
	e->picture = avcodec_alloc_frame();
	if (!e->picture)
		goto bail;
	/* The idea here is that the encoded frame that will land in this
	 * buffer will be no larger than the size of an uncompressed 32-bit
	 * rgb frame.
	 *
	 * TODO: Is this assumption really valid?
	 */
	e->frame_buf_len = w * h * 4;
	e->frame_buf = malloc(e->frame_buf_len);
	if (!e->frame_buf)
		goto bail;

	c->decstate = calloc(sizeof(struct decoder_ctx), 1);
	if (!c->decstate)
		goto bail;
	d = c->decstate;
	d->avctx = avcodec_alloc_context();
	if (!d->avctx)
		goto bail;
	d->picture = avcodec_alloc_frame();
	if (!d->picture)
		goto bail;
	d->frame_buf_len = e->frame_buf_len;
	d->frame_buf = malloc(d->frame_buf_len);
	if (!d->frame_buf)
		goto bail;

	e->slice_header.version = 0;
	srandom(time(0));
	e->slice_header.source_id = random() & 0xffff;

	e->avctx->time_base.num = 1;
	e->avctx->time_base.den = framerate;

	e->avctx->width = w;
	e->avctx->height = h;

	e->avctx->bit_rate = bitrate;

	/* This determines how often i-frames are sent */
	e->avctx->gop_size = framerate * 3;
	e->avctx->pix_fmt = PIX_FMT_YUV420P;
	e->avctx->has_b_frames = 0;

	e->avctx->mb_qmin = e->avctx->qmin = 10;
	e->avctx->mb_qmax = e->avctx->qmax = 10;

	e->avctx->lmin = 2 * FF_QP2LAMBDA;
	e->avctx->lmax = 10 * FF_QP2LAMBDA;
	e->avctx->global_quality = FF_QP2LAMBDA * 2;
	e->avctx->qblur = 0.5;
	e->avctx->global_quality = 10;

	e->avctx->flags |= CODEC_FLAG_PSNR;
	e->avctx->flags |= CODEC_FLAG_QSCALE;

	e->avctx->mb_decision = FF_MB_DECISION_SIMPLE;

	ff_enc_id = ff_dec_id = map_iaxc_codec_to_avcodec(format);

	/* Note, when fragsize is used (non-zero) ffmpeg will use a "best
	 * effort" strategy: the fragment size will be fragsize +/- 20%
	 */

	switch (format)
	{

	case IAXC_FORMAT_H261:
		/* TODO: H261 only works with specific resolutions. */
		name = "H.261";
		break;

	case IAXC_FORMAT_H263:
		/* TODO: H263 only works with specific resolutions. */
		name = "H.263";
		e->avctx->flags |= CODEC_FLAG_AC_PRED;
		if (fragsize)
		{
			c->decode = decode_rtp_slice;
			e->avctx->rtp_payload_size = fragsize;
			e->avctx->flags |=
				CODEC_FLAG_TRUNCATED | CODEC_FLAG2_STRICT_GOP;
			e->avctx->rtp_callback = encode_rtp_callback;
			d->avctx->flags |= CODEC_FLAG_TRUNCATED;
		}
		break;

	case IAXC_FORMAT_H263_PLUS:
		/* Although the encoder is CODEC_ID_H263P, the decoder
		 * is the regular h.263, so we handle this special case
		 * here.
		 */
		ff_dec_id = CODEC_ID_H263;
		name = "H.263+";
		e->avctx->flags |= CODEC_FLAG_AC_PRED;
		if (fragsize)
		{
			c->decode = decode_rtp_slice;
			e->avctx->rtp_payload_size = fragsize;
			e->avctx->flags |=
				CODEC_FLAG_TRUNCATED |
				CODEC_FLAG_H263P_SLICE_STRUCT |
				CODEC_FLAG2_STRICT_GOP |
				CODEC_FLAG2_LOCAL_HEADER;
			e->avctx->rtp_callback = encode_rtp_callback;
			d->avctx->flags |= CODEC_FLAG_TRUNCATED;
		}
		break;

	case IAXC_FORMAT_MPEG4:
		name = "MPEG4";
		c->decode = decode_rtp_slice;
		e->avctx->rtp_payload_size = fragsize;
		e->avctx->rtp_callback = encode_rtp_callback;
		e->avctx->flags |=
			CODEC_FLAG_TRUNCATED |
			CODEC_FLAG_H263P_SLICE_STRUCT |
			CODEC_FLAG2_STRICT_GOP |
			CODEC_FLAG2_LOCAL_HEADER;

		d->avctx->flags |= CODEC_FLAG_TRUNCATED;
		break;

	case IAXC_FORMAT_H264:
		name = "H.264";

		/*
		 * Encoder flags
		 */

		/* Headers are not repeated */
		/* e->avctx->flags |= CODEC_FLAG_GLOBAL_HEADER; */

		/* Slower, less blocky */
		/* e->avctx->flags |= CODEC_FLAG_LOOP_FILTER; */

		e->avctx->flags |= CODEC_FLAG_PASS1;
		/* e->avctx->flags |= CODEC_FLAG_PASS2; */

		/* Compute psnr values at encode-time (avctx->error[]) */
		/* e->avctx->flags |= CODEC_FLAG_PSNR; */

		/* e->avctx->flags2 |= CODEC_FLAG2_8X8DCT; */

		/* Access Unit Delimiters */
		e->avctx->flags2 |= CODEC_FLAG2_AUD;

		/* Allow b-frames to be used as reference */
		/* e->avctx->flags2 |= CODEC_FLAG2_BPYRAMID; */

		/* b-frame rate distortion optimization */
		/* e->avctx->flags2 |= CODEC_FLAG2_BRDO; */

		/* e->avctx->flags2 |= CODEC_FLAG2_FASTPSKIP; */

		/* Multiple references per partition */
		/* e->avctx->flags2 |= CODEC_FLAG2_MIXED_REFS; */

		/* Weighted biprediction for b-frames */
		/* e->avctx->flags2 |= CODEC_FLAG2_WPRED; */

		/*
		 * Decoder flags
		 */

		/* Do not draw edges */
		/* d->avctx->flags |= CODEC_FLAG_EMU_EDGE; */

		/* Decode grayscale only */
		/* d->avctx->flags |= CODEC_FLAG_GRAY; */

		/* d->avctx->flags |= CODEC_FLAG_LOW_DELAY; */

		/* Allow input bitstream to be randomly truncated */
		/* d->avctx->flags |= CODEC_FLAG_TRUNCATED; */

		/* Allow out-of-spec speed tricks */
		/* d->avctx->flags2 |= CODEC_FLAG2_FAST; */
		break;

	case IAXC_FORMAT_THEORA:
		/* TODO: ffmpeg only has a theora decoder. Until it has
		 * an encoder also, we cannot use ffmpeg for theora.
		 */
		name = "Theora";
		break;

	default:
		fprintf(stderr, "codec_ffmpeg: unsupported format (0x%08x)\n",
				format);
		goto bail;
	}

	strcpy(c->name, "ffmpeg-");
	strncat(c->name, name, sizeof(c->name));

	/* Get the codecs */
	codec = avcodec_find_encoder(ff_enc_id);
	if (!codec)
	{
		iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
			     "codec_ffmpeg: cannot find encoder %d\n",
			     ff_enc_id);
		goto bail;
	}

	if (avcodec_open(e->avctx, codec))
	{
		iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
			     "codec_ffmpeg: cannot open encoder %s\n", name);
		goto bail;
	}

	codec = avcodec_find_decoder(ff_dec_id);
	if (!codec)
	{
		iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
			     "codec_ffmpeg: cannot find decoder %d\n",
			     ff_dec_id);
		goto bail;
	}
	if (avcodec_open(d->avctx, codec))
	{
		iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
			     "codec_ffmpeg: cannot open decoder %s\n", name);
		goto bail;
	}

	{
		enum PixelFormat fmts[] = { PIX_FMT_YUV420P, -1 };
		if (d->avctx->get_format(d->avctx, fmts) != PIX_FMT_YUV420P)
		{
			iaxci_usermsg(IAXC_TEXT_TYPE_ERROR,
					"codec_ffmpeg: cannot set decode format to YUV420P\n");
			goto bail;
		}
	}

	return c;

bail:
	destroy(c);
	return 0;
}

int codec_video_ffmpeg_check_codec(int format)
{
	AVCodec *codec;
	enum CodecID codec_id;

	/* These functions are idempotent, so it is okay that we
	 * may call them elsewhere at a different time.
	 */
	avcodec_init();
	avcodec_register_all();

	codec_id = map_iaxc_codec_to_avcodec(format);

	if (codec_id == CODEC_ID_NONE)
		return 0;

	codec = avcodec_find_encoder(codec_id);

	return codec ? 1 : 0;
}

