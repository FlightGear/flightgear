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

#include "cst_synth.h"
#include "cst_utt_utils.h"
#include "cst_math.h"
#include "cst_file.h"
#include "cst_val.h"
#include "cst_string.h"
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_tokenstream.h"
#include "cst_string.h"
#include "cst_regex.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "flite.h"
#include "cst_synth.h"
#include "cst_utt_utils.h"

#include "flite_hts_engine.h"

/* HTS_GStreamSet_get_total_nsamples: get total number of sample */
size_t HTS_GStreamSet_get_total_nsamples(HTS_GStreamSet * gss);

/* HTS_GStreamSet_get_speech: get synthesized speech parameter */
double HTS_GStreamSet_get_speech(HTS_GStreamSet * gss, size_t sample_index);

#define REGISTER_VOX register_cmu_us_kal
#define UNREGISTER_VOX unregister_cmu_us_kal

#define MAXBUFLEN 1024

cst_voice *REGISTER_VOX(const char *voxdir);
cst_voice *UNREGISTER_VOX(cst_voice * vox);

/* Flite_HTS_Engine_create_label: create label per phoneme */
static void Flite_HTS_Engine_create_label(Flite_HTS_Engine * f, cst_item * item, char *label)
{
   char seg_pp[8];
   char seg_p[8];
   char seg_c[8];
   char seg_n[8];
   char seg_nn[8];
   char endtone[8];
   int sub_phrases = 0;
   int lisp_total_phrases = 0;
   int tmp1 = 0;
   int tmp2 = 0;
   int tmp3 = 0;
   int tmp4 = 0;

   /* load segments */
   strcpy(seg_pp, ffeature_string(item, "p.p.name"));
   strcpy(seg_p, ffeature_string(item, "p.name"));
   strcpy(seg_c, ffeature_string(item, "name"));
   strcpy(seg_n, ffeature_string(item, "n.name"));
   strcpy(seg_nn, ffeature_string(item, "n.n.name"));

   /* load endtone */
   strcpy(endtone, ffeature_string(item, "R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.endtone"));

   if (strcmp(seg_c, "pau") == 0) {
      /* for pause */
      if (item_next(item) != NULL) {
         sub_phrases = ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.sub_phrases");
         tmp1 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         tmp2 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         lisp_total_phrases = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      } else {
         sub_phrases = ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.sub_phrases");
         tmp1 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         tmp2 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         lisp_total_phrases = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      }
      sprintf(label, "%s^%s-%s+%s=%s@x_x/A:%d_%d_%d/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:%d+%d+%d/D:%s_%d/E:x+x@x+x&x+x#x+x/F:%s_%d/G:%d_%d/H:x=x^%d=%d|%s/I:%d=%d/J:%d+%d-%d", strcmp(seg_pp, "0") == 0 ? "x" : seg_pp, strcmp(seg_p, "0") == 0 ? "x" : seg_p, seg_c, strcmp(seg_n, "0") == 0 ? "x" : seg_n, strcmp(seg_nn, "0") == 0 ? "x" : seg_nn, ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.stress"), ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.accented"), ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.syl_numphones"), ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.stress"), ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.accented"), ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.syl_numphones"), ffeature_string(item, "p.R:SylStructure.parent.parent.R:Word.gpos"), ffeature_int(item, "p.R:SylStructure.parent.parent.R:Word.word_numsyls"), ffeature_string(item, "n.R:SylStructure.parent.parent.R:Word.gpos"), ffeature_int(item, "n.R:SylStructure.parent.parent.R:Word.word_numsyls"), ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"), ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"), sub_phrases + 1, lisp_total_phrases - sub_phrases, endtone, ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"), ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"), tmp1, tmp2, lisp_total_phrases);
   } else {
      /* for no pause */
      tmp1 = ffeature_int(item, "R:SylStructure.pos_in_syl");
      tmp2 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_numphones");
      tmp3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.pos_in_word");
      tmp4 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.word_numsyls");
      sub_phrases = ffeature_int(item, "R:SylStructure.parent.R:Syllable.sub_phrases");
      lisp_total_phrases = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      sprintf(label, "%s^%s-%s+%s=%s@%d_%d/A:%d_%d_%d/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%d-%d;%d-%d|%s/C:%d+%d+%d/D:%s_%d/E:%s+%d@%d+%d&%d+%d#%d+%d/F:%s_%d/G:%d_%d/H:%d=%d^%d=%d|%s/I:%d=%d/J:%d+%d-%d", strcmp(seg_pp, "0") == 0 ? "x" : seg_pp, strcmp(seg_p, "0") == 0 ? "x" : seg_p, seg_c, strcmp(seg_n, "0") == 0 ? "x" : seg_n, strcmp(seg_nn, "0") == 0 ? "x" : seg_nn, tmp1 + 1, tmp2 - tmp1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.stress"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.accented"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.syl_numphones"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.stress"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.accented"), tmp2, tmp3 + 1, tmp4 - tmp3, ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_in") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_out") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_in") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_out") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_in") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_out") + 1, ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_stress"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_stress"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_accent"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_accent"), ffeature_string(item, "R:SylStructure.parent.R:Syllable.syl_vowel"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.stress"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.accented"), ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.syl_numphones"), ffeature_string(item, "R:SylStructure.parent.parent.R:Word.p.gpos"), ffeature_int(item, "R:SylStructure.parent.parent.R:Word.p.word_numsyls"), ffeature_string(item, "R:SylStructure.parent.parent.R:Word.gpos"), tmp4, ffeature_int(item, "R:SylStructure.parent.parent.R:Word.pos_in_phrase") + 1, ffeature_int(item, "R:SylStructure.parent.parent.R:Word.words_out"), ffeature_int(item, "R:SylStructure.parent.parent.R:Word.content_words_in") + 1, ffeature_int(item, "R:SylStructure.parent.parent.R:Word.content_words_out") + 1, ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_p_content"), ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_n_content"), ffeature_string(item, "R:SylStructure.parent.parent.R:Word.n.gpos"), ffeature_int(item, "R:SylStructure.parent.parent.R:Word.n.word_numsyls"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_syls_in_phrase"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_words_in_phrase"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"), sub_phrases + 1, lisp_total_phrases - sub_phrases, strcmp(endtone, "0") == 0 ? "NONE" : endtone, ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_syls_in_phrase"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_words_in_phrase"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls"), ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words"), lisp_total_phrases);
   }
}

/* Flite_HTS_Engine_initialize: initialize system */
void Flite_HTS_Engine_initialize(Flite_HTS_Engine * f)
{
   HTS_Engine_initialize(&f->engine);
}

/* Flite_HTS_Engine_load: load HTS voice */
HTS_Boolean Flite_HTS_Engine_load(Flite_HTS_Engine * f, const char *fn)
{
   HTS_Boolean result;
   char *voices = strdup(fn);
   result = HTS_Engine_load(&f->engine, &voices, 1);
   free(voices);
   return result;
}

/* Flite_HTS_Engine_set_sampling_frequency: set sampling frequency */
void Flite_HTS_Engine_set_sampling_frequency(Flite_HTS_Engine * f, size_t i)
{
   HTS_Engine_set_sampling_frequency(&f->engine, i);
}

/* Flite_HTS_Engine_set_fperiod: set frame period */
void Flite_HTS_Engine_set_fperiod(Flite_HTS_Engine * f, size_t i)
{
   HTS_Engine_set_fperiod(&f->engine, i);
}

/* Flite_HTS_Engine_set_audio_buff_size: set audio buffer size */
void Flite_HTS_Engine_set_audio_buff_size(Flite_HTS_Engine * f, size_t i)
{
   HTS_Engine_set_audio_buff_size(&f->engine, i);
}

/* Flite_HTS_Engine_set_alpha: set alpha */
void Flite_HTS_Engine_set_alpha(Flite_HTS_Engine * f, double d)
{
   HTS_Engine_set_alpha(&f->engine, d);
}

/* Flite_HTS_Engine_set_beta: set beta */
void Flite_HTS_Engine_set_beta(Flite_HTS_Engine * f, double d)
{
   HTS_Engine_set_beta(&f->engine, d);
}

/* Flite_HTS_Engine_add_half_tone: add half-tone */
void Flite_HTS_Engine_add_half_tone(Flite_HTS_Engine * f, double d)
{
   HTS_Engine_add_half_tone(&f->engine, d);
}

/* Flite_HTS_Engine_set_msd_threshold: set MSD threshold */
void Flite_HTS_Engine_set_msd_threshold(Flite_HTS_Engine * f, size_t stream_index, double d)
{
   HTS_Engine_set_msd_threshold(&f->engine, stream_index, d);
}

/* Flite_HTS_Engine_set_gv_weight: set GV weight */
void Flite_HTS_Engine_set_gv_weight(Flite_HTS_Engine * f, size_t stream_index, double d)
{
   HTS_Engine_set_gv_weight(&f->engine, stream_index, d);
}

/* Flite_HTS_Engine_set_speed: set speech speed */
void Flite_HTS_Engine_set_speed(Flite_HTS_Engine * f, double d)
{
   HTS_Engine_set_speed(&f->engine, d);
}

/* Flite_HTS_Engine_synthesize: synthesize speech */
HTS_Boolean Flite_HTS_Engine_synthesize(Flite_HTS_Engine * f, const char *txt, const char *wav)
{
   int i;
   FILE *fp;
   cst_voice *v = NULL;
   cst_utterance *u = NULL;
   cst_item *s = NULL;
   char **label_data = NULL;
   int label_size = 0;

   if (txt == NULL)
      return FALSE;

   /* text analysis part */
   v = REGISTER_VOX(NULL);
   if (v == NULL)
      return FALSE;
   u = flite_synth_text(txt, v);
   if (u == NULL)
      return FALSE;
   for (s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s))
      label_size++;
   if (label_size <= 0)
      return FALSE;
   label_data = (char **) calloc(label_size, sizeof(char *));
   for (i = 0, s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s), i++) {
      label_data[i] = (char *) calloc(MAXBUFLEN, sizeof(char));
      Flite_HTS_Engine_create_label(f, s, label_data[i]);
   }

   /* speech synthesis part */
   HTS_Engine_synthesize_from_strings(&f->engine, label_data, label_size);
   if (wav != NULL) {
      fp = fopen(wav, "wb");
      HTS_Engine_save_riff(&f->engine, fp);
      fclose(fp);
   }
   HTS_Engine_refresh(&f->engine);

   for (i = 0; i < label_size; i++)
      free(label_data[i]);
   free(label_data);

   delete_utterance(u);
   UNREGISTER_VOX(v);

   return TRUE;
}

/* Flite_HTS_Engine_synthesize: synthesize speech */
HTS_Boolean Flite_HTS_Engine_synthesize_samples_mono16(Flite_HTS_Engine * f, const char *txt,
                                                void** samples, int* sampleCount, int* sampleRate)
{
    int i;
    cst_voice *v = NULL;
    cst_utterance *u = NULL;
    cst_item *s = NULL;
    char **label_data = NULL;
    int label_size = 0;
    short* samplePtr = NULL;
    HTS_GStreamSet *gss;
    
    if (txt == NULL)
        return FALSE;
    
    /* text analysis part */
    v = REGISTER_VOX(NULL);
    if (v == NULL)
        return FALSE;
    u = flite_synth_text(txt, v);
    if (u == NULL)
        return FALSE;
    for (s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s))
        label_size++;
    if (label_size <= 0)
        return FALSE;
    label_data = (char **) calloc(label_size, sizeof(char *));
    for (i = 0, s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s), i++) {
        label_data[i] = (char *) calloc(MAXBUFLEN, sizeof(char));
        Flite_HTS_Engine_create_label(f, s, label_data[i]);
    }
    
    /* speech synthesis part */
    HTS_Engine_synthesize_from_strings(&f->engine, label_data, label_size);
    
    gss = &f->engine.gss;
    *sampleRate = f->engine.condition.sampling_frequency;
    *sampleCount = HTS_GStreamSet_get_total_nsamples(gss);
    *samples = malloc(sizeof(short) * *sampleCount);
    samplePtr = *samples;
    
    for (i=0; i < *sampleCount; ++i) {
        *samplePtr++ = (short) HTS_GStreamSet_get_speech(gss, i);
    }
    
    HTS_Engine_refresh(&f->engine);
    
    for (i = 0; i < label_size; i++)
        free(label_data[i]);
    free(label_data);
    
    delete_utterance(u);
    UNREGISTER_VOX(v);
    
    return TRUE;
}


/* Flite_HTS_Engine_clear: free system */
void Flite_HTS_Engine_clear(Flite_HTS_Engine * f)
{
   HTS_Engine_clear(&f->engine);
}

typedef struct _Flite_Utterance {
   cst_voice *v;
   cst_utterance *u;
   int nitem;
   cst_item **items;
} Flite_Utterance;

/* Flite_Text_Analyzer_initialize: initialize flite front-end */
void Flite_Text_Analyzer_initialize(Flite_Text_Analyzer * analyzer)
{
   if (analyzer == NULL)
      return;
   analyzer->pointer = NULL;
}

/* Flite_Text_Analyzer_analysis: text analysis */
void Flite_Text_Analyzer_analysis(Flite_Text_Analyzer * analyzer, const char *text)
{
   int i;
   cst_item *s;
   Flite_Utterance *fu;

   if (analyzer == NULL || text == NULL)
      return;

   if (analyzer->pointer != NULL)
      Flite_Text_Analyzer_clear(analyzer);

   /* allocate */
   fu = (Flite_Utterance *) malloc(sizeof(Flite_Utterance));
   if (fu == NULL)
      return;

   /* create voice */
   fu->v = REGISTER_VOX(NULL);
   if (fu->v == NULL) {
      free(fu);
      return;
   }

   /* create utterance */
   fu->u = flite_synth_text(text, fu->v);
   if (fu->u == NULL) {
      UNREGISTER_VOX(fu->v);
      free(fu);
      return;
   }

   /* count number of phonemes */
   for (fu->nitem = 0, s = relation_head(utt_relation(fu->u, "Segment")); s; s = item_next(s), fu->nitem++);
   if (fu->nitem == 0) {
      delete_utterance(fu->u);
      UNREGISTER_VOX(fu->v);
      free(fu);
      return;
   }

   /* save informations */
   fu->items = (cst_item **) malloc(sizeof(cst_item *) * fu->nitem);
   for (i = 0, s = relation_head(utt_relation(fu->u, "Segment")); s; s = item_next(s), i++)
      fu->items[i] = s;

   analyzer->pointer = (void *) fu;
}

/* Flite_Text_Analyzer_get_nphoneme_in_utterance: get number of phonemes */
int Flite_Text_Analyzer_get_nphoneme_in_utterance(Flite_Text_Analyzer * analyzer)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;

   fu = (Flite_Utterance *) analyzer->pointer;
   return fu->nitem;
}

/* Flite_Text_Analyzer_get_phoneme: get phoneme identity */
const char *Flite_Text_Analyzer_get_phoneme(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return NULL;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return NULL;
   return ffeature_string(fu->items[phoneme_index], "name");
}

/* Flite_Text_Analyzer_get_word: get word */
const char *Flite_Text_Analyzer_get_word(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return NULL;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return NULL;
   return ffeature_string(fu->items[phoneme_index], "R:SylStructure.parent.parent.name");
}

/* Flite_Text_Analyzer_get_nphoneme_in_syllable: get number of phonemes in syllable */
int Flite_Text_Analyzer_get_nphoneme_in_syllable(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.R:Syllable.syl_numphones");
}

/* Flite_Text_Analayzer_get_nsyllable_in_word: get number of syllables in word */
int Flite_Text_Analyzer_get_nsyllable_in_word(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.parent.R:Word.word_numsyls");
}

/* Flite_Text_Analyzer_get_nword_in_phrase: get number of words in phrase */
int Flite_Text_Analyzer_get_nword_in_phrase(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase");
}

/* Flite_Text_Analyzer_get_nphrase_in_utterance: get number of phrases in utterance */
int Flite_Text_Analyzer_get_nphrase_in_utterance(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
}

/* Flite_Text_Analyzer_get_accent: get accent */
int Flite_Text_Analyzer_get_accent(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.R:Syllable.accented");
}

/* Flite_Text_Analyzer_get_stress: get stress */
int Flite_Text_Analyzer_get_stress(Flite_Text_Analyzer * analyzer, int phoneme_index)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return 0;
   fu = (Flite_Utterance *) analyzer->pointer;
   if (phoneme_index < 0 || phoneme_index >= fu->nitem)
      return 0;
   return ffeature_int(fu->items[phoneme_index], "R:SylStructure.parent.R:Syllable.stress");
}

/* Flite_Text_Analyzer_clear: finalize flite front-end */
void Flite_Text_Analyzer_clear(Flite_Text_Analyzer * analyzer)
{
   Flite_Utterance *fu;

   if (analyzer == NULL || analyzer->pointer == NULL)
      return;

   fu = (Flite_Utterance *) analyzer->pointer;
   if (fu->items != NULL)
      free(fu->items);
   if (fu->u != NULL)
      delete_utterance(fu->u);
   if (fu->v != NULL)
      UNREGISTER_VOX(fu->v);
   free(fu);

   analyzer->pointer = NULL;
}
