
EXTRA_DIST = Makefile.mak

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/flite/include \
           -I$(top_srcdir)/flite/lang/cmu_us_kal \
           -I$(top_srcdir)/flite/lang/cmulex \
           -I$(top_srcdir)/flite/lang/usenglish \
           -I$(HTS_ENGINE_HEADER_DIR)

lib_LIBRARIES = libflhtse.a

libflhtse_a_SOURCES = flite_hts_engine.c \
                      $(top_srcdir)/flite/include/cst_alloc.h \
                      $(top_srcdir)/flite/include/cst_args.h \
                      $(top_srcdir)/flite/include/cst_audio.h \
                      $(top_srcdir)/flite/include/cst_cart.h \
                      $(top_srcdir)/flite/include/cst_endian.h \
                      $(top_srcdir)/flite/include/cst_error.h \
                      $(top_srcdir)/flite/include/cst_features.h \
                      $(top_srcdir)/flite/include/cst_ffeatures.h \
                      $(top_srcdir)/flite/include/cst_file.h \
                      $(top_srcdir)/flite/include/cst_hrg.h \
                      $(top_srcdir)/flite/include/cst_item.h \
                      $(top_srcdir)/flite/include/cst_lexicon.h \
                      $(top_srcdir)/flite/include/cst_lts.h \
                      $(top_srcdir)/flite/include/cst_lts_rewrites.h \
                      $(top_srcdir)/flite/include/cst_math.h \
                      $(top_srcdir)/flite/include/cst_phoneset.h \
                      $(top_srcdir)/flite/include/cst_regex.h \
                      $(top_srcdir)/flite/include/cst_relation.h \
                      $(top_srcdir)/flite/include/cst_sigpr.h \
                      $(top_srcdir)/flite/include/cst_socket.h \
                      $(top_srcdir)/flite/include/cst_ss.h \
                      $(top_srcdir)/flite/include/cst_string.h \
                      $(top_srcdir)/flite/include/cst_sts.h \
                      $(top_srcdir)/flite/include/cst_synth.h \
                      $(top_srcdir)/flite/include/cst_tokenstream.h \
                      $(top_srcdir)/flite/include/cst_track.h \
                      $(top_srcdir)/flite/include/cst_utt_utils.h \
                      $(top_srcdir)/flite/include/cst_utterance.h \
                      $(top_srcdir)/flite/include/cst_val.h \
                      $(top_srcdir)/flite/include/cst_val_const.h \
                      $(top_srcdir)/flite/include/cst_val_defs.h \
                      $(top_srcdir)/flite/include/cst_viterbi.h \
                      $(top_srcdir)/flite/include/cst_voice.h \
                      $(top_srcdir)/flite/include/cst_wave.h \
                      $(top_srcdir)/flite/include/flite.h \
                      $(top_srcdir)/flite/lang/cmu_us_kal/cmu_us_kal.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lex.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lex.h \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lex_data.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lex_entries.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lts_model.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lts_model.h \
                      $(top_srcdir)/flite/lang/cmulex/cmu_lts_rules.c \
                      $(top_srcdir)/flite/lang/cmulex/cmu_postlex.c \
                      $(top_srcdir)/flite/lang/usenglish/us_aswd.c \
                      $(top_srcdir)/flite/lang/usenglish/us_expand.c \
                      $(top_srcdir)/flite/lang/usenglish/us_ffeatures.c \
                      $(top_srcdir)/flite/lang/usenglish/us_ffeatures.h \
                      $(top_srcdir)/flite/lang/usenglish/us_gpos.c \
                      $(top_srcdir)/flite/lang/usenglish/us_int_accent_cart.c \
                      $(top_srcdir)/flite/lang/usenglish/us_int_accent_cart.h \
                      $(top_srcdir)/flite/lang/usenglish/us_int_tone_cart.c \
                      $(top_srcdir)/flite/lang/usenglish/us_int_tone_cart.h \
                      $(top_srcdir)/flite/lang/usenglish/us_nums_cart.c \
                      $(top_srcdir)/flite/lang/usenglish/us_nums_cart.h \
                      $(top_srcdir)/flite/lang/usenglish/us_phoneset.c \
                      $(top_srcdir)/flite/lang/usenglish/us_phrasing_cart.c \
                      $(top_srcdir)/flite/lang/usenglish/us_phrasing_cart.h \
                      $(top_srcdir)/flite/lang/usenglish/us_pos_cart.c \
                      $(top_srcdir)/flite/lang/usenglish/us_pos_cart.h \
                      $(top_srcdir)/flite/lang/usenglish/us_regexes.h \
                      $(top_srcdir)/flite/lang/usenglish/us_text.c \
                      $(top_srcdir)/flite/lang/usenglish/us_text.h \
                      $(top_srcdir)/flite/lang/usenglish/usenglish.c \
                      $(top_srcdir)/flite/lang/usenglish/usenglish.h \
                      $(top_srcdir)/flite/src/hrg/cst_ffeature.c \
                      $(top_srcdir)/flite/src/hrg/cst_item.c \
                      $(top_srcdir)/flite/src/hrg/cst_relation.c \
                      $(top_srcdir)/flite/src/hrg/cst_utterance.c \
                      $(top_srcdir)/flite/src/lexicon/cst_lexicon.c \
                      $(top_srcdir)/flite/src/lexicon/cst_lts.c \
                      $(top_srcdir)/flite/src/regex/cst_regex.c \
                      $(top_srcdir)/flite/src/regex/cst_regex_defs.h \
                      $(top_srcdir)/flite/src/regex/regexp.c \
                      $(top_srcdir)/flite/src/stats/cst_cart.c \
                      $(top_srcdir)/flite/src/synth/cst_ffeatures.c \
                      $(top_srcdir)/flite/src/synth/cst_phoneset.c \
                      $(top_srcdir)/flite/src/synth/cst_synth.c \
                      $(top_srcdir)/flite/src/synth/cst_utt_utils.c \
                      $(top_srcdir)/flite/src/synth/cst_voice.c \
                      $(top_srcdir)/flite/src/synth/flite.c \
                      $(top_srcdir)/flite/src/utils/cst_alloc.c \
                      $(top_srcdir)/flite/src/utils/cst_error.c \
                      $(top_srcdir)/flite/src/utils/cst_features.c \
                      $(top_srcdir)/flite/src/utils/cst_string.c \
                      $(top_srcdir)/flite/src/utils/cst_tokenstream.c \
                      $(top_srcdir)/flite/src/utils/cst_val.c \
                      $(top_srcdir)/flite/src/utils/cst_val_const.c \
                      $(top_srcdir)/flite/src/utils/cst_val_user.c \
                      $(top_srcdir)/include/flite_hts_engine.h

DISTCLEANFILES = *.log *.out *~
