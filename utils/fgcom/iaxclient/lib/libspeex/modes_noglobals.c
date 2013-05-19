/* Copyright (C) 2004 CSIRO Australia
   File: modes_noglobals.c

   Hacked by Conrad Parker, based on modes.c:
   Copyright (C) 2002 Jean-Marc Valin 

   Describes the different modes of the codec. This file differs from
   modes.c in that SpeexMode structures are dynamically allocated,
   rather than being statically defined. This introduces some minor
   API changes which are described in the file README.symbian in the
   top level of the Speex source distribution.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "modes.h"
#include "ltp.h"
#include "quant_lsp.h"
#include "cb_search.h"
#include "sb_celp.h"
#include "nb_celp.h"
#include "vbr.h"
#include "misc.h"
#include <math.h>
#include <string.h>

#ifndef NULL
#define NULL 0
#endif

/* Extern declarations for all codebooks we use here */
extern const signed char gain_cdbk_nb[];
extern const signed char gain_cdbk_lbr[];
extern const signed char hexc_table[];
extern const signed char exc_5_256_table[];
extern const signed char exc_5_64_table[];
extern const signed char exc_8_128_table[];
extern const signed char exc_10_32_table[];
extern const signed char exc_10_16_table[];
extern const signed char exc_20_32_table[];
extern const signed char hexc_10_32_table[];

static const ltp_params *
speex_ltp_params_new (const signed char * gain_cdbk, int gain_bits,
		      int pitch_bits)
{
  ltp_params * params;

  params = (ltp_params *) speex_alloc (sizeof (ltp_params));
  if (params == NULL) return NULL;

  params->gain_cdbk = gain_cdbk;
  params->gain_bits = gain_bits;
  params->pitch_bits = pitch_bits;

  return params;
}

static void
speex_ltp_params_free (const ltp_params * params)
{
  speex_free ((void *)params);
}

static const split_cb_params *
speex_split_cb_params_new (int subvect_size, int nb_subvect,
			   const signed char * shape_cb, int shape_bits,
			   int have_sign)
{
  split_cb_params * params;

  params = (split_cb_params *) speex_alloc (sizeof (split_cb_params));
  if (params == NULL) return NULL;

  params->subvect_size = subvect_size;
  params->nb_subvect = nb_subvect;
  params->shape_cb = shape_cb;
  params->shape_bits = shape_bits;
  params->have_sign = have_sign;

  return params;
}

static void
speex_split_cb_params_free (const split_cb_params * params)
{
  speex_free ((void *)params);
}

static SpeexSubmode *
speex_submode_new (int lbr_pitch, int forced_pitch_gain,
		   int have_subframe_gain, int double_codebook,

		   lsp_quant_func lsp_quant, lsp_unquant_func lsp_unquant,
		   ltp_quant_func ltp_quant, ltp_unquant_func ltp_unquant,
		   const void * ltp_params,

		   innovation_quant_func innovation_quant,
		   innovation_unquant_func innovation_unquant,
		   const void * innovation_params,

		   /*Synthesis filter enhancement*/
		   spx_word16_t      lpc_enh_k1, /**< Enhancer constant */
		   spx_word16_t      lpc_enh_k2, /**< Enhancer constant */
		   spx_word16_t      lpc_enh_k3, /**< Enhancer constant */
		   spx_word16_t      comb_gain,  /**< Gain of enhancer comb filter */
		   
		   int               bits_per_frame /**< Number of bits per frame after encoding*/

		   )
{
  SpeexSubmode * submode;

  submode = (SpeexSubmode *) speex_alloc (sizeof (SpeexSubmode));
  if (submode == NULL) return NULL;

  submode->lbr_pitch = lbr_pitch;
  submode->forced_pitch_gain = forced_pitch_gain;
  submode->have_subframe_gain = have_subframe_gain;
  submode->double_codebook = double_codebook;
  submode->lsp_quant = lsp_quant;
  submode->lsp_unquant = lsp_unquant;
  submode->ltp_quant = ltp_quant;
  submode->ltp_unquant = ltp_unquant;
  submode->ltp_params = ltp_params;
  submode->innovation_quant = innovation_quant;
  submode->innovation_unquant = innovation_unquant;
  submode->innovation_params = innovation_params;
  submode->lpc_enh_k1 = lpc_enh_k1;
  submode->lpc_enh_k2 = lpc_enh_k2;
  submode->lpc_enh_k3 = lpc_enh_k3;
  submode->comb_gain = comb_gain;
  submode->bits_per_frame = bits_per_frame;

  return submode;
}

static void
speex_submode_free (const SpeexSubmode * submode)
{
  if (submode->ltp_params)
    speex_ltp_params_free (submode->ltp_params);

  if (submode->innovation_params)
    speex_split_cb_params_free (submode->innovation_params);

  speex_free ((void *)submode);
}

static SpeexNBMode *
nb_mode_new (int frameSize, int subframeSize, int lpcSize, int bufSize,
	     int pitchStart, int pitchEnd, spx_word16_t gamma1,
	     spx_word16_t gamma2, float lag_factor, float lpc_floor,
#ifdef EPIC_48K
	     int lbr48k,
#endif
	     const SpeexSubmode * submodes[], int defaultSubmode,
	     int quality_map[])
{
  SpeexNBMode * nb_mode;

  nb_mode = (SpeexNBMode *) speex_alloc (sizeof (SpeexNBMode));
  if (nb_mode == NULL) return NULL;

  nb_mode->frameSize = frameSize;
  nb_mode->subframeSize = subframeSize;
  nb_mode->lpcSize = lpcSize;
  nb_mode->bufSize = bufSize;
  nb_mode->pitchStart = pitchStart;
  nb_mode->pitchEnd = pitchEnd;
  nb_mode->gamma1 = gamma1;
  nb_mode->gamma2 = gamma2;
  nb_mode->lag_factor = lag_factor;
  nb_mode->lpc_floor = lpc_floor;
#ifdef EPIC_48K
  nb_mode->lbr48k = lbr48k;
#endif
  memcpy (nb_mode->submodes, submodes, sizeof (nb_mode->submodes));
  nb_mode->defaultSubmode = defaultSubmode;
  memcpy (nb_mode->quality_map, quality_map, sizeof (nb_mode->quality_map));

  return nb_mode;
}

static void
nb_mode_free (const SpeexNBMode * nb_mode)
{
  speex_free ((void *)nb_mode);
}

static SpeexSBMode *
sb_mode_new (
   const SpeexMode *nb_mode,    /**< Embedded narrowband mode */
   int     frameSize,     /**< Size of frames used for encoding */
   int     subframeSize,  /**< Size of sub-frames used for encoding */
   int     lpcSize,       /**< Order of LPC filter */
   int     bufSize,       /**< Signal buffer size in encoder */
   spx_word16_t gamma1,   /**< Perceptual filter parameter #1 */
   spx_word16_t gamma2,   /**< Perceptual filter parameter #1 */
   float   lag_factor,    /**< Lag-windowing parameter */
   float   lpc_floor,     /**< Noise floor for LPC analysis */
   float   folding_gain,

   const SpeexSubmode *submodes[], /**< Sub-mode data for the mode */
   int     defaultSubmode, /**< Default sub-mode to use when encoding */
   int     low_quality_map[], /**< Mode corresponding to each quality setting */
   int     quality_map[], /**< Mode corresponding to each quality setting */
   const float (*vbr_thresh)[11],
   int     nb_modes
		   )
{
  SpeexSBMode * sb_mode;

  sb_mode = (SpeexSBMode *) speex_alloc (sizeof (SpeexSBMode));
  if (sb_mode == NULL) return NULL;

  sb_mode->nb_mode = nb_mode;
  sb_mode->frameSize = frameSize;
  sb_mode->subframeSize = subframeSize;
  sb_mode->lpcSize = lpcSize;
  sb_mode->bufSize = bufSize;
  sb_mode->gamma1 = gamma1;
  sb_mode->gamma2 = gamma2;
  sb_mode->lag_factor = lag_factor;
  sb_mode->lpc_floor = lpc_floor;
  sb_mode->folding_gain = folding_gain;

  memcpy (sb_mode->submodes, submodes, sizeof (sb_mode->submodes));
  sb_mode->defaultSubmode = defaultSubmode;
  memcpy (sb_mode->low_quality_map, low_quality_map, sizeof (sb_mode->low_quality_map));
  memcpy (sb_mode->quality_map, quality_map, sizeof (sb_mode->quality_map));
  sb_mode->vbr_thresh = vbr_thresh;
  sb_mode->nb_modes = nb_modes;

  return sb_mode;
}

static void
sb_mode_free (const SpeexSBMode * sb_mode)
{
  int i;

  for (i = 0; i < SB_SUBMODES; i++)
    if (sb_mode->submodes[i]) speex_submode_free (sb_mode->submodes[i]);

  speex_free ((void *)sb_mode);
}

static SpeexMode *
mode_new (const void * b_mode, mode_query_func query, char * modeName,
	  int modeID, int bitstream_version, encoder_init_func enc_init,
	  encoder_destroy_func enc_destroy, encode_func enc,
	  decoder_init_func dec_init, decoder_destroy_func dec_destroy,
	  decode_func dec, encoder_ctl_func enc_ctl,
	  decoder_ctl_func dec_ctl)
{
  SpeexMode * mode;

  mode = (SpeexMode *) speex_alloc (sizeof (SpeexMode));
  if (mode == NULL) return NULL;

  mode->mode = b_mode;
  mode->query = query;
  mode->modeName = modeName;
  mode->modeID = modeID;
  mode->bitstream_version = bitstream_version;
  mode->enc_init = enc_init;
  mode->enc_destroy = enc_destroy;
  mode->enc = enc;
  mode->dec_init = dec_init;
  mode->dec_destroy = dec_destroy;
  mode->dec = dec;
  mode->enc_ctl = enc_ctl;
  mode->dec_ctl = dec_ctl;

  return mode;
}

/* Freeing each kind of created (SpeexMode *) is done separately below */

/* Parameters for Long-Term Prediction (LTP)*/
static const ltp_params * ltp_params_nb (void)
{
  return speex_ltp_params_new (
   gain_cdbk_nb,
   7,
   7
   );
}

/* Parameters for Long-Term Prediction (LTP)*/
static const ltp_params * ltp_params_vlbr (void)
{
  return speex_ltp_params_new (
   gain_cdbk_lbr,
   5,
   0
   );
}

/* Parameters for Long-Term Prediction (LTP)*/
static const ltp_params * ltp_params_lbr (void)
{
  return speex_ltp_params_new (
   gain_cdbk_lbr,
   5,
   7
   );
}

/* Parameters for Long-Term Prediction (LTP)*/
static const ltp_params * ltp_params_med (void)
{
  return speex_ltp_params_new (
   gain_cdbk_lbr,
   5,
   7
   );
}

/* Split-VQ innovation parameters for very low bit-rate narrowband */
static const split_cb_params * split_cb_nb_vlbr (void)
{
  return speex_split_cb_params_new (
   10,               /*subvect_size*/
   4,               /*nb_subvect*/
   exc_10_16_table, /*shape_cb*/
   4,               /*shape_bits*/
   0
   );
}

/* Split-VQ innovation parameters for very low bit-rate narrowband */
static const split_cb_params * split_cb_nb_ulbr (void)
{
  return speex_split_cb_params_new (
   20,               /*subvect_size*/
   2,               /*nb_subvect*/
   exc_20_32_table, /*shape_cb*/
   5,               /*shape_bits*/
   0
   );
}

/* Split-VQ innovation parameters for low bit-rate narrowband */
static const split_cb_params * split_cb_nb_lbr (void)
{
  return speex_split_cb_params_new (
   10,              /*subvect_size*/
   4,               /*nb_subvect*/
   exc_10_32_table, /*shape_cb*/
   5,               /*shape_bits*/
   0
   );
}


/* Split-VQ innovation parameters narrowband */
static const split_cb_params * split_cb_nb (void)
{
  return speex_split_cb_params_new (
   5,               /*subvect_size*/
   8,               /*nb_subvect*/
   exc_5_64_table, /*shape_cb*/
   6,               /*shape_bits*/
   0
   );
}

/* Split-VQ innovation parameters narrowband */
static const split_cb_params * split_cb_nb_med (void)
{
  return speex_split_cb_params_new (
   8,               /*subvect_size*/
   5,               /*nb_subvect*/
   exc_8_128_table, /*shape_cb*/
   7,               /*shape_bits*/
   0
   );
}

/* Split-VQ innovation for low-band wideband */
static const split_cb_params * split_cb_sb (void)
{
  return speex_split_cb_params_new (
   5,               /*subvect_size*/
   8,              /*nb_subvect*/
   exc_5_256_table,    /*shape_cb*/
   8,               /*shape_bits*/
   0
   );
}

/* Split-VQ innovation for high-band wideband */
static const split_cb_params * split_cb_high (void)
{
  return speex_split_cb_params_new (
   8,               /*subvect_size*/
   5,               /*nb_subvect*/
   hexc_table,       /*shape_cb*/
   7,               /*shape_bits*/
   1
   );
}


/* Split-VQ innovation for high-band wideband */
static const split_cb_params * split_cb_high_lbr (void)
{
  return speex_split_cb_params_new (
   10,               /*subvect_size*/
   4,               /*nb_subvect*/
   hexc_10_32_table,       /*shape_cb*/
   5,               /*shape_bits*/
   0
   );
}

/* 2150 bps "vocoder-like" mode for comfort noise */
static const SpeexSubmode * nb_submode1 (void)
{
  return speex_submode_new (
   0,
   1,
   0,
   0,
   /* LSP quantization */
   lsp_quant_lbr,
   lsp_unquant_lbr,
   /* No pitch quantization */
   forced_pitch_quant,
   forced_pitch_unquant,
   NULL,
   /* No innovation quantization (noise only) */
   noise_codebook_quant,
   noise_codebook_unquant,
   NULL,
#ifdef FIXED_POINT
   22938, 22938, 0, -1,
#else
   .7, .7, 0, -1,
#endif
   43
   );
}

/* 3.95 kbps very low bit-rate mode */
static const SpeexSubmode * nb_submode8 (void)
{
  const split_cb_params * params;

  params = split_cb_nb_ulbr();
  if (params == NULL) return NULL;

  return speex_submode_new (
   0,
   1,
   0,
   0,
   /*LSP quantization*/
   lsp_quant_lbr,
   lsp_unquant_lbr,
   /*No pitch quantization*/
   forced_pitch_quant,
   forced_pitch_unquant,
   NULL,
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   params,
#ifdef FIXED_POINT
   22938, 16384, 11796, 21299,
#else
   0.7, 0.5, .36, .65,
#endif
   79
   );
}

/* 5.95 kbps very low bit-rate mode */
static const SpeexSubmode * nb_submode2 (void)
{
  return speex_submode_new (
   0,
   0,
   0,
   0,
   /*LSP quantization*/
   lsp_quant_lbr,
   lsp_unquant_lbr,
   /*No pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_vlbr(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_nb_vlbr(),
#ifdef FIXED_POINT
   22938, 16384, 11796, 18022,
#else
   0.7, 0.5, .36, .55,
#endif
   119
   );
}

/* 8 kbps low bit-rate mode */
static const SpeexSubmode * nb_submode3 (void)
{
  return speex_submode_new (
   -1,
   0,
   1,
   0,
   /*LSP quantization*/
   lsp_quant_lbr,
   lsp_unquant_lbr,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_lbr(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_nb_lbr(),
#ifdef FIXED_POINT
   22938, 18022, 9830, 14746,
#else
   0.7, 0.55, .30, .45,
#endif
   160
   );
}

/* 11 kbps medium bit-rate mode */
static const SpeexSubmode * nb_submode4 (void)
{
  return speex_submode_new (
   -1,
   0,
   1,
   0,
   /*LSP quantization*/
   lsp_quant_lbr,
   lsp_unquant_lbr,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_med(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_nb_med(),
#ifdef FIXED_POINT
   22938, 20644, 5243, 11469,
#else
   0.7, 0.63, .16, .35,
#endif
   220
   );
}

/* 15 kbps high bit-rate mode */
static const SpeexSubmode * nb_submode5 (void)
{
  return speex_submode_new (
   -1,
   0,
   3,
   0,
   /*LSP quantization*/
   lsp_quant_nb,
   lsp_unquant_nb,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_nb(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_nb(),
#ifdef FIXED_POINT
   22938, 21299, 3932, 8192,
#else
   0.7, 0.65, .12, .25,
#endif
   300
   );
}

/* 18.2 high bit-rate mode */
static const SpeexSubmode * nb_submode6 (void)
{
  return speex_submode_new (
   -1,
   0,
   3,
   0,
   /*LSP quantization*/
   lsp_quant_nb,
   lsp_unquant_nb,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_nb(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_sb(),
#ifdef FIXED_POINT
   22282, 21299, 2294, 3277,
#else
   0.68, 0.65, .07, .1,
#endif
   364
   );
}

/* 24.6 kbps high bit-rate mode */
static const SpeexSubmode * nb_submode7 (void)
{
  return speex_submode_new (
   -1,
   0,
   3,
   1,
   /*LSP quantization*/
   lsp_quant_nb,
   lsp_unquant_nb,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   ltp_params_nb(),
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_nb(),
#ifdef FIXED_POINT
   21299, 21299, 0, -1,
#else
   0.65, 0.65, .0, -1,
#endif
   492
   );
}


/* Default mode for narrowband */
static const SpeexNBMode * nb_mode (void)
{
  const SpeexSubmode ** submodes;
  int quality_map[11] = {1, 8, 2, 3, 3, 4, 4, 5, 5, 6, 7};
  const SpeexNBMode * ret;

  submodes = (const SpeexSubmode **)
    speex_alloc (sizeof (SpeexSubmode *) * SB_SUBMODES);
  if (submodes == NULL) return NULL;
  memset (submodes, 0, sizeof (submodes));

  if (!(submodes[1] = nb_submode1())) goto nb_1;
  if (!(submodes[2] = nb_submode2())) goto nb_2;
  if (!(submodes[3] = nb_submode3())) goto nb_3;
  if (!(submodes[4] = nb_submode4())) goto nb_4;
  if (!(submodes[5] = nb_submode5())) goto nb_5;
  if (!(submodes[6] = nb_submode6())) goto nb_6;
  if (!(submodes[7] = nb_submode7())) goto nb_7;
  if (!(submodes[8] = nb_submode8())) goto nb_8;

  ret = nb_mode_new (
   160,    /*frameSize*/
   40,     /*subframeSize*/
   10,     /*lpcSize*/
   640,    /*bufSize*/
   17,     /*pitchStart*/
   144,    /*pitchEnd*/
#ifdef FIXED_POINT
   29491, 19661, /* gamma1, gamma2 */
#else
   0.9, 0.6, /* gamma1, gamma2 */
#endif
   .012,   /*lag_factor*/
   1.0002, /*lpc_floor*/
#ifdef EPIC_48K
   0,
#endif
   submodes,
   5,
   quality_map
   );

  if (ret == NULL) goto nb_8;
 
  /* If nb_mode_new() was successful, the references to submodes have been
   * copied into ret->submodes[], and it's safe to free submodes.
   */
  speex_free ((void *)submodes);

  return ret;

  /* Cleanup on memory allocation errors */
 nb_8: speex_submode_free (submodes[8]);
 nb_7: speex_submode_free (submodes[7]);
 nb_6: speex_submode_free (submodes[6]);
 nb_5: speex_submode_free (submodes[5]);
 nb_4: speex_submode_free (submodes[4]);
 nb_3: speex_submode_free (submodes[3]);
 nb_2: speex_submode_free (submodes[2]);
 nb_1: speex_submode_free (submodes[1]);

  speex_free ((void *)submodes);

  return NULL;
}


/* Default mode for narrowband */
static const SpeexMode * speex_nb_mode_new (void)
{
  const SpeexNBMode * _nb_mode;

  _nb_mode = nb_mode();
  if (_nb_mode == NULL) return NULL;

  return mode_new (
   _nb_mode,
   nb_mode_query,
   "narrowband",
   0,
   4,
   &nb_encoder_init,
   &nb_encoder_destroy,
   &nb_encode,
   &nb_decoder_init,
   &nb_decoder_destroy,
   &nb_decode,
   &nb_encoder_ctl,
   &nb_decoder_ctl
   );
}

static void speex_nb_mode_free (const SpeexMode * mode)
{
  nb_mode_free ((SpeexNBMode *)mode->mode);
  speex_free ((void *)mode);
}

/* Wideband part */

static const SpeexSubmode * wb_submode1 (void)
{
  return speex_submode_new (
   0,
   0,
   1,
   0,
   /*LSP quantization*/
   lsp_quant_high,
   lsp_unquant_high,
   /*Pitch quantization*/
   NULL,
   NULL,
   NULL,
   /*No innovation quantization*/
   NULL,
   NULL,
   NULL,
#ifdef FIXED_POINT
   24576, 24576, 0, -1,
#else
   .75, .75, .0, -1,
#endif
   36
   );
}


static const SpeexSubmode * wb_submode2 (void)
{
  return speex_submode_new (
   0,
   0,
   1,
   0,
   /*LSP quantization*/
   lsp_quant_high,
   lsp_unquant_high,
   /*Pitch quantization*/
   NULL,
   NULL,
   NULL,
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_high_lbr(),
#ifdef FIXED_POINT
   27853, 19661, 8192, -1,
#else
   .85, .6, .25, -1,
#endif
   112
   );
}


static const SpeexSubmode * wb_submode3 (void)
{
  return speex_submode_new (
   0,
   0,
   1,
   0,
   /*LSP quantization*/
   lsp_quant_high,
   lsp_unquant_high,
   /*Pitch quantization*/
   NULL,
   NULL,
   NULL,
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_high(),

#ifdef FIXED_POINT
   24576, 22938, 1638, -1,
#else
   .75, .7, .05, -1,
#endif
   192
   );
}

static const SpeexSubmode * wb_submode4 (void)
{
  return speex_submode_new (
   0,
   0,
   1,
   1,
   /*LSP quantization*/
   lsp_quant_high,
   lsp_unquant_high,
   /*Pitch quantization*/
   NULL,
   NULL,
   NULL,
   /*Innovation quantization*/
   split_cb_search_shape_sign,
   split_cb_shape_sign_unquant,
   split_cb_high(),
#ifdef FIXED_POINT
   24576, 24576, 0, -1,
#else
   .75, .75, .0, -1,
#endif
   352
   );
}


/* Split-band wideband CELP mode*/
static const SpeexSBMode * sb_wb_mode (void)
{
  const SpeexMode * nb_mode;
  const SpeexSubmode ** submodes;
  int low_quality_map[11] = {1, 8, 2, 3, 4, 5, 5, 6, 6, 7, 7};
  int quality_map[11] = {1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4};
  SpeexSBMode * ret;

  nb_mode = speex_nb_mode_new ();
  if (nb_mode == NULL) return NULL;

  submodes = (const SpeexSubmode **)
    speex_alloc (sizeof (SpeexSubmode *) * SB_SUBMODES);
  if (submodes == NULL) return NULL;
  memset (submodes, 0, sizeof (submodes));

  if (!(submodes[1] = wb_submode1())) goto sb_1;
  if (!(submodes[2] = wb_submode2())) goto sb_2;
  if (!(submodes[3] = wb_submode3())) goto sb_3;
  if (!(submodes[4] = wb_submode4())) goto sb_4;

  ret = sb_mode_new (
   nb_mode,
   160,    /*frameSize*/
   40,     /*subframeSize*/
   8,     /*lpcSize*/
   640,    /*bufSize*/
#ifdef FIXED_POINT
   29491, 19661, /* gamma1, gamma2 */
#else
   0.9, 0.6, /* gamma1, gamma2 */
#endif
   .001,   /*lag_factor*/
   1.0001, /*lpc_floor*/
   0.9,
   submodes,
   3,
   low_quality_map,
   quality_map,
   vbr_hb_thresh,
   5
   );

  if (ret == NULL) goto sb_4;

  /* If sb_mode_new() was successful, the references to submodes have been
   * copied into ret->submodes[], and it's safe to free submodes.
   */
  speex_free ((void *)submodes);

  return ret;

  /* Cleanup on memory allocation errors */
 sb_4: speex_submode_free (submodes[4]);
 sb_3: speex_submode_free (submodes[3]);
 sb_2: speex_submode_free (submodes[2]);
 sb_1: speex_submode_free (submodes[1]);

  speex_free ((void *)submodes);

  return NULL;
}

static void
sb_wb_mode_free (const SpeexSBMode * mode)
{
  speex_nb_mode_free (mode->nb_mode);
}

static const SpeexMode * speex_wb_mode_new (void)
{
  const SpeexSBMode * sb_mode;

  sb_mode = sb_wb_mode ();
  if (sb_mode == NULL) return NULL;

  return mode_new (
   (const SpeexNBMode *)sb_mode,
   wb_mode_query,
   "wideband (sub-band CELP)",
   1,
   4,
   &sb_encoder_init,
   &sb_encoder_destroy,
   &sb_encode,
   &sb_decoder_init,
   &sb_decoder_destroy,
   &sb_decode,
   &sb_encoder_ctl,
   &sb_decoder_ctl
   );
}

static void speex_wb_mode_free (const SpeexMode * mode)
{
  sb_wb_mode_free (mode->mode);
  speex_free ((void *)mode);
}


/* "Ultra-wideband" mode stuff */



/* Split-band "ultra-wideband" (32 kbps) CELP mode*/
static const SpeexSBMode * sb_uwb_mode (void)
{
  const SpeexSBMode * nb_mode;
  const SpeexSubmode ** submodes;
  int low_quality_map[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int quality_map[11] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  SpeexSBMode * ret;

  nb_mode = sb_wb_mode ();
  if (nb_mode == NULL) return NULL;

  submodes = (const SpeexSubmode **)
    speex_alloc (sizeof (SpeexSubmode *) * SB_SUBMODES);
  if (submodes == NULL) return NULL;
  memset (submodes, 0, sizeof (submodes));

  if (!(submodes[1] = wb_submode1())) goto uwb_1;

  ret = sb_mode_new (
   (const SpeexMode *)nb_mode,
   320,    /*frameSize*/
   80,     /*subframeSize*/
   8,     /*lpcSize*/
   1280,    /*bufSize*/
#ifdef FIXED_POINT
   29491, 19661, /* gamma1, gamma2 */
#else
   0.9, 0.6, /* gamma1, gamma2 */
#endif
   .002,   /*lag_factor*/
   1.0001, /*lpc_floor*/
   0.7,
   submodes,
   1,
   low_quality_map,
   quality_map,
   vbr_uhb_thresh,
   2
   );

  if (ret == NULL) goto uwb_1;

  /* If sb_mode_new() was successful, the references to submodes have been
   * copied into ret->submodes[], and it's safe to free submodes.
   */
  speex_free ((void *)submodes);

  return ret;

 uwb_1: speex_submode_free (submodes[1]);

  speex_free ((void *)submodes);

  return NULL;
}

static void sb_uwb_mode_free (const SpeexSBMode * mode)
{
  sb_wb_mode_free ((const SpeexSBMode *)mode->nb_mode);
  sb_mode_free (mode);
}

static const SpeexMode * speex_uwb_mode_new (void)
{
  const SpeexSBMode * sb_mode;

  sb_mode = sb_uwb_mode();
  if (sb_mode == NULL) return NULL;

  return mode_new (
   sb_mode,
   wb_mode_query,
   "ultra-wideband (sub-band CELP)",
   2,
   4,
   &sb_encoder_init,
   &sb_encoder_destroy,
   &sb_encode,
   &sb_decoder_init,
   &sb_decoder_destroy,
   &sb_decode,
   &sb_encoder_ctl,
   &sb_decoder_ctl
   );
}

static void speex_uwb_mode_free (const SpeexMode * mode)
{
  sb_uwb_mode_free (mode->mode);
  speex_free ((void *)mode);
}

const SpeexMode * speex_mode_new (int modeID)
{
  switch (modeID) {
  case 0: return speex_nb_mode_new(); break;
  case 1: return speex_wb_mode_new(); break;
  case 2: return speex_uwb_mode_new(); break;
  default: return NULL;
  }
}

void speex_mode_destroy (const SpeexMode * mode)
{
  switch (mode->modeID) {
  case 0: speex_nb_mode_free(mode); break;
  case 1: speex_wb_mode_free(mode); break;
  case 2:  speex_uwb_mode_free(mode); break;
  default: break;
  }
}

/** XXX: This is just a dummy global mode, as used by nb_celp.c */
const SpeexMode speex_wb_mode = {
   NULL,
   NULL,
   NULL,
   0,
   0,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};

int speex_mode_query(const SpeexMode *mode, int request, void *ptr)
{
  if (mode == &speex_wb_mode && request == SPEEX_SUBMODE_BITS_PER_FRAME) {
    int * p = (int*)ptr;

    switch (*p) {
    case 0: *p = SB_SUBMODE_BITS+1; break;
    case 1: *p = 36; break;
    case 2: *p = 112; break;
    case 3: *p = 192; break;
    case 4: *p = 352; break;
    default: *p = -1; break;
    }

    return 0;
  }
  
  return mode->query(mode->mode, request, ptr);
}
