/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Light weight run-time speech synthesis system, public API            */
/*                                                                       */
/*************************************************************************/

/* ----------------------------------------------------------------- */
/*           The English TTS System "Flite+hts_engine"               */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2005-2013  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2005-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#ifndef _FLITE_H__
#define _FLITE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cst_string.h"
#include "cst_regex.h"
#include "cst_val.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_wave.h"
#include "cst_track.h"

#include "cst_cart.h"
#include "cst_phoneset.h"
#include "cst_voice.h"
#include "cst_audio.h"

#include "cst_utt_utils.h"
#include "cst_lexicon.h"
#include "cst_synth.h"
#ifndef FLITE_PLUS_HTS_ENGINE
#include "cst_units.h"
#endif /* !FLITE_PLUS_HTS_ENGINE */
#include "cst_tokenstream.h"

extern cst_val *flite_voice_list;

/* Public functions */
int flite_init();

/* General top level functions */
cst_voice *flite_voice_select(const char *name);
float flite_file_to_speech(const char *filename, 
			   cst_voice *voice,
			   const char *outtype);
float flite_text_to_speech(const char *text, 
			   cst_voice *voice,
			   const char *outtype);
float flite_phones_to_speech(const char *text, 
			     cst_voice *voice,
			     const char *outtype);
float flite_ssml_to_speech(const char *filename,
                           cst_voice *voice,
                           const char *outtype);
int flite_voice_add_lex_addenda(cst_voice *v, const cst_string *lexfile);

/* Lower lever user functions */
cst_wave *flite_text_to_wave(const char *text,cst_voice *voice);
cst_utterance *flite_synth_text(const char *text,cst_voice *voice);
cst_utterance *flite_synth_phones(const char *phones,cst_voice *voice);

cst_utterance *flite_do_synth(cst_utterance *u,
                              cst_voice *voice,
                              cst_uttfunc synth);
float flite_process_output(cst_utterance *u,
                           const char *outtype,
                           int append);

/* flite public export wrappers for features access */
int flite_get_param_int(const cst_features *f, const char *name,int def);
float flite_get_param_float(const cst_features *f, const char *name, float def);
const char *flite_get_param_string(const cst_features *f, const char *name, const char *def);
const cst_val *flite_get_param_val(const cst_features *f, const char *name, cst_val *def);
void flite_feat_set_int(cst_features *f, const char *name, int v);
void flite_feat_set_float(cst_features *f, const char *name, float v);
void flite_feat_set_string(cst_features *f, const char *name, const char *v);
void flite_feat_set(cst_features *f, const char *name,const cst_val *v);
int flite_feat_remove(cst_features *f, const char *name);

const char *flite_ffeature_string(const cst_item *item,const char *featpath);
int flite_ffeature_int(const cst_item *item,const char *featpath);
float flite_ffeature_float(const cst_item *item,const char *featpath);
const cst_val *flite_ffeature(const cst_item *item,const char *featpath);
cst_item* flite_path_to_item(const cst_item *item,const char *featpath);

#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */


#endif
