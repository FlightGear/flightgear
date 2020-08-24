if (SYSTEM_FLITE)
    set(Flite_FIND_REQUIRED TRUE)
    find_package(Flite)

    message(STATUS "Using Flite includes at: ${FLITE_INCLUDE_DIR}")
    message(STATUS "Using Flite libraries: ${FLITE_LIBRARIES}")
else()
    set(FLITE_SOURCES
        flite/src/hrg/cst_ffeature.c
        flite/src/hrg/cst_item.c
        flite/src/hrg/cst_relation.c
        flite/src/hrg/cst_utterance.c

        flite/src/lexicon/cst_lexicon.c
        flite/src/lexicon/cst_lts.c

        flite/src/regex/cst_regex.c
        flite/src/regex/regexp.c

        flite/src/stats/cst_cart.c

        flite/src/synth/cst_ffeatures.c
        flite/src/synth/cst_phoneset.c
        flite/src/synth/cst_synth.c
        flite/src/synth/cst_utt_utils.c
        flite/src/synth/cst_voice.c
        flite/src/synth/flite.c

        flite/src/utils/cst_alloc.c
        flite/src/utils/cst_error.c
        flite/src/utils/cst_features.c
        flite/src/utils/cst_string.c
        flite/src/utils/cst_tokenstream.c
        flite/src/utils/cst_val.c
        flite/src/utils/cst_val_const.c
        flite/src/utils/cst_val_user.c
    )

    set(FLITE_LANG_SOURCES
        flite/lang/cmu_us_kal/cmu_us_kal.c

        flite/lang/cmulex/cmu_lex.c
        flite/lang/cmulex/cmu_lex_data.c
        flite/lang/cmulex/cmu_lex_entries.c
        flite/lang/cmulex/cmu_lts_model.c
        flite/lang/cmulex/cmu_lts_rules.c
        flite/lang/cmulex/cmu_postlex.c

        flite/lang/usenglish/us_aswd.c
        flite/lang/usenglish/us_expand.c
        flite/lang/usenglish/us_ffeatures.c
        flite/lang/usenglish/us_gpos.c
        flite/lang/usenglish/us_int_accent_cart.c
        flite/lang/usenglish/us_int_tone_cart.c
        flite/lang/usenglish/us_nums_cart.c
        flite/lang/usenglish/us_phoneset.c
        flite/lang/usenglish/us_phrasing_cart.c
        flite/lang/usenglish/us_pos_cart.c
        flite/lang/usenglish/us_text.c
        flite/lang/usenglish/usenglish.c
    )

endif()

add_library(flightgear_flite_hts STATIC 
    ${FLITE_SOURCES}
    ${FLITE_LANG_SOURCES}
    lib/flite_hts_engine.c
)

target_include_directories(flightgear_flite_hts PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/include)
target_compile_definitions(flightgear_flite_hts PRIVATE -DNO_UNION_INITIALIZATION -DFLITE_PLUS_HTS_ENGINE)

if (X86_64)
    target_compile_definitions(flightgear_flite_hts PRIVATE -DSIZEOF_VOID_P=8)
endif()

# link against HTSEngine: either our own one, or the system one
target_link_libraries(flightgear_flite_hts PUBLIC Flightgear::HTSEngine)

if (SYSTEM_FLITE)
    target_link_libraries(flightgear_flite_hts PUBLIC Flightgear::Flite)
else()
    target_include_directories(flightgear_flite_hts PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/include)
    target_include_directories(flightgear_flite_hts PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/lang/cmulex)
    target_include_directories(flightgear_flite_hts PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/lang/usenglish)
endif()


