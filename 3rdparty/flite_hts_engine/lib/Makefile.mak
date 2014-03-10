
CC		= cl
CL		= link

CFLAGS = /I C:\hts_engine_API\include /I ..\include /I ..\flite\include \
         /I ..\flite\lang\cmu_us_kal /I ..\flite\lang\cmulex /I ..\flite\lang\usenglish \
         /D "FLITE_PLUS_HTS_ENGINE" /D "NO_UNION_INITIALIZATION" \
         /O2 /Ob2 /Oi /Ot /Oy /GT /TC /GL
LFLAGS = /LTCG

CORES = flite_hts_engine.obj \
        ..\flite\lang\cmu_us_kal\cmu_us_kal.obj \
        ..\flite\lang\cmulex\cmu_lex.obj \
        ..\flite\lang\cmulex\cmu_lex_data.obj \
        ..\flite\lang\cmulex\cmu_lex_entries.obj \
        ..\flite\lang\cmulex\cmu_lts_model.obj \
        ..\flite\lang\cmulex\cmu_lts_rules.obj \
        ..\flite\lang\usenglish\us_aswd.obj \
        ..\flite\lang\usenglish\us_expand.obj \
        ..\flite\lang\usenglish\us_ffeatures.obj \
        ..\flite\lang\usenglish\us_gpos.obj \
        ..\flite\lang\usenglish\us_int_accent_cart.obj \
        ..\flite\lang\usenglish\us_int_tone_cart.obj \
        ..\flite\lang\usenglish\us_nums_cart.obj \
        ..\flite\lang\usenglish\us_phoneset.obj \
        ..\flite\lang\usenglish\us_phrasing_cart.obj \
        ..\flite\lang\usenglish\us_postlex.obj \
        ..\flite\lang\usenglish\us_text.obj \
        ..\flite\lang\usenglish\usenglish.obj \
        ..\flite\src\hrg\cst_ffeature.obj \
        ..\flite\src\hrg\cst_item.obj \
        ..\flite\src\hrg\cst_relation.obj \
        ..\flite\src\hrg\cst_utterance.obj \
        ..\flite\src\lexicon\cst_lexicon.obj \
        ..\flite\src\lexicon\cst_lts.obj \
        ..\flite\src\regex\cst_regex.obj \
        ..\flite\src\regex\regexp.obj \
        ..\flite\src\stats\cst_cart.obj \
        ..\flite\src\synth\cst_phoneset.obj \
        ..\flite\src\synth\cst_synth.obj \
        ..\flite\src\synth\cst_utt_utils.obj \
        ..\flite\src\synth\cst_voice.obj \
        ..\flite\src\synth\flite.obj \
        ..\flite\src\utils\cst_alloc.obj \
        ..\flite\src\utils\cst_error.obj \
        ..\flite\src\utils\cst_features.obj \
        ..\flite\src\utils\cst_string.obj \
        ..\flite\src\utils\cst_tokenstream.obj \
        ..\flite\src\utils\cst_val.obj \
        ..\flite\src\utils\cst_val_const.obj \
        ..\flite\src\utils\cst_val_user.obj

all: flite_hts_engine.lib

flite_hts_engine.lib: $(CORES)
	lib $(LFLAGS) /OUT:$@ *.obj

.c.obj:
	$(CC) $(CFLAGS) /c $<

clean:
	del *.lib
	del $(CORES) 
