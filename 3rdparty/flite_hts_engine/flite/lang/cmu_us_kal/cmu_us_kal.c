/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2000                          */
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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*  A simple voice defintion                                             */
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

#include "flite.h"
#ifndef FLITE_PLUS_HTS_ENGINE
#include "cst_diphone.h"
#endif /* !FLITE_PLUS_HTS_ENGINE */
#include "usenglish.h"
#include "cmu_lex.h"

static cst_utterance *cmu_us_kal_postlex(cst_utterance *u);
#ifndef FLITE_PLUS_HTS_ENGINE
extern cst_diphone_db cmu_us_kal_db;
#endif /* !FLITE_PLUS_HTS_ENGINE */

cst_voice *cmu_us_kal_diphone = NULL;

cst_voice *register_cmu_us_kal(const char *voxdir)
{
    cst_voice *v;
    cst_lexicon *lex;

    if (cmu_us_kal_diphone)
        return cmu_us_kal_diphone;  /* Already registered */
    
    v  = new_voice();
    v->name = "kal";

    /* Set up basic values for synthesizing with this voice */
    usenglish_init(v);
#ifndef FLITE_PLUS_HTS_ENGINE
    flite_feat_set_string(v->features,"name","cmu_us_kal_diphone");
#endif /* !FLITE_PLUS_HTS_ENGINE */

    /* Lexicon */
    lex = cmu_lex_init();
    flite_feat_set(v->features,"lexicon",lexicon_val(lex));

#ifndef FLITE_PLUS_HTS_ENGINE
    /* Intonation */
    flite_feat_set_float(v->features,"int_f0_target_mean",95.0);
    flite_feat_set_float(v->features,"int_f0_target_stddev",11.0);

    flite_feat_set_float(v->features,"duration_stretch",1.1); 
#endif /* !FLITE_PLUS_HTS_ENGINE */

    /* Post lexical rules */
    flite_feat_set(v->features,"postlex_func",uttfunc_val(&cmu_us_kal_postlex));

#ifndef FLITE_PLUS_HTS_ENGINE
    /* Waveform synthesis: diphone_synth */
    flite_feat_set(v->features,"wave_synth_func",uttfunc_val(&diphone_synth));
    flite_feat_set(v->features,"diphone_db",diphone_db_val(&cmu_us_kal_db));
    flite_feat_set_int(v->features,"sample_rate",cmu_us_kal_db.sts->sample_rate);
    flite_feat_set_string(v->features,"resynth_type","fixed");
    flite_feat_set_string(v->features,"join_type","modified_lpc");
#endif /* !FLITE_PLUS_HTS_ENGINE */

    cmu_us_kal_diphone = v;

    return cmu_us_kal_diphone;
}

void unregister_cmu_us_kal(cst_voice *v)
{
    if (v != cmu_us_kal_diphone)
	return;
    delete_voice(v);
    cmu_us_kal_diphone = NULL;
}

#ifndef FLITE_PLUS_HTS_ENGINE
static void fix_ah(cst_utterance *u)
{
    /* This should really be done in the index itself */
    const cst_item *s;

    for (s=relation_head(utt_relation(u,"Segment")); s; s=item_next(s))
	if (cst_streq(item_feat_string(s,"name"),"ah"))
	    item_set_string(s,"name","aa");
}
#endif /* !FLITE_PLUS_HTS_ENGINE */

static cst_utterance *cmu_us_kal_postlex(cst_utterance *u)
{
    cmu_postlex(u);
#ifndef FLITE_PLUS_HTS_ENGINE
    fix_ah(u);
#endif /* !FLITE_PLUS_HTS_ENGINE */

    return u;
}
