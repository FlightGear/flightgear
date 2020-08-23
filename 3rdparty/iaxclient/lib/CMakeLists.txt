
set(IAXCLIENT_BASE_SOURCES
    audio_encode.c
    audio_file.c
    audio_openal.c
    codec_alaw.c
    codec_gsm.c
    codec_ulaw.c
    codec_speex.c
    iaxclient_lib.c
    spandsp/plc.c
)

set(LIBIAX2_SOURCES
    libiax2/src/iax.c
    libiax2/src/iax2-parser.c
    libiax2/src/jitterbuf.c
    libiax2/src/md5.c
)

find_package(OpenAL REQUIRED)

add_library(iaxclient_lib STATIC 
    ${IAXCLIENT_BASE_SOURCES} ${LIBIAX2_SOURCES}
)

target_link_libraries(iaxclient_lib PRIVATE Threads::Threads ${OPENAL_LIBRARY})
target_include_directories(iaxclient_lib PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/iaxclient/lib)
target_include_directories(iaxclient_lib PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/iaxclient/lib/libiax2/src ${OPENAL_INCLUDE_DIR})

target_compile_definitions(iaxclient_lib PRIVATE AUDIO_OPENAL=1 LIBIAX LIBVER="SVN 261")

if (APPLE)
    target_compile_definitions(iaxclient_lib PRIVATE MACOSX)
endif(APPLE)

if (WIN32)
    target_sources(iaxclient_lib PRIVATE winfuncs.c)
else()
    target_sources(iaxclient_lib PRIVATE unixfuncs.c)
endif(WIN32)


if (MSVC)
    # turn off various warnings
    set(WFLAGS)
    foreach(warning 4244 4305 4996)
        list(APPEND WFLAGS "/wd${warning}")
    endforeach()
    target_compile_definitions(iaxclient_lib PRIVATE ${WFLAGS})
endif ()

if (SYSTEM_GSM)
    set(Gsm_FIND_REQUIRED TRUE)
    find_package(Gsm)
endif(SYSTEM_GSM)

if (GSM_FOUND)
    message(STATUS "Using GSM includes at: ${GSM_INCLUDE_DIR}")
    message(STATUS "Using GSM libraries: ${GSM_LIBRARIES}")
    target_include_directories(iaxclient_lib PRIVATE ${GSM_INCLUDE_DIR})
    target_link_libraries(iaxclient_lib PRIVATE ${GSM_LIBRARIES})
else()
    # add local GSM files
    target_sources(iaxclient_lib PRIVATE
        gsm/src/add.c
        gsm/src/code.c
        gsm/src/debug.c
        gsm/src/decode.c
        gsm/src/gsm_create.c
        gsm/src/gsm_decode.c
        gsm/src/gsm_destroy.c
        gsm/src/gsm_encode.c
        gsm/src/gsm_explode.c
        gsm/src/gsm_implode.c
        gsm/src/gsm_option.c
        gsm/src/gsm_print.c
        gsm/src/long_term.c
        gsm/src/lpc.c
        gsm/src/preprocess.c
        gsm/src/rpe.c
        gsm/src/short_term.c
        gsm/src/table.c
    )

    target_include_directories(iaxclient_lib PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/iaxclient/lib/gsm/inc)
    target_compile_definitions(iaxclient_lib PRIVATE HAS_STRING_H HAS_STDLIB_H)
endif()

target_compile_definitions(iaxclient_lib PRIVATE CODEC_GSM)

if (SYSTEM_SPEEX)
    set(Speex_FIND_REQUIRED TRUE)
    set(Speexdsp_FIND_REQUIRED TRUE)
    find_package(Speex)
    find_package(Speexdsp)
endif()

if (SPEEX_FOUND AND SPEEXDSP_FOUND)
    
    message(STATUS "Using speex includes at: ${SPEEX_INCLUDE_DIR}")
    message(STATUS "Using speex libraries: ${SPEEX_LIBRARIES}")
    message(STATUS "Using speex extended library includes at: ${SPEEXDSP_INCLUDE_DIR}")
    message(STATUS "Using speex extended library libraries: ${SPEEXDSP_LIBRARIES}")

    target_include_directories(iaxclient_lib PRIVATE ${SPEEX_INCLUDE_DIR} ${SPEEXDSP_INCLUDE_DIR})
    target_link_libraries(iaxclient_lib PRIVATE ${SPEEX_LIBRARIES} ${SPEEXDSP_LIBRARIES})
else()
    target_sources(iaxclient_lib PRIVATE
        libspeex/bits.c
        libspeex/cb_search.c
        libspeex/exc_10_16_table.c
        libspeex/exc_10_32_table.c
        libspeex/exc_20_32_table.c
        libspeex/exc_5_256_table.c
        libspeex/exc_5_64_table.c
        libspeex/exc_8_128_table.c
        libspeex/filters.c
        libspeex/gain_table.c
        libspeex/gain_table_lbr.c
        libspeex/hexc_10_32_table.c
        libspeex/hexc_table.c
        libspeex/high_lsp_tables.c
        libspeex/jitter.c
        libspeex/lbr_48k_tables.c
        libspeex/lpc.c
        libspeex/lsp.c
        libspeex/lsp_tables_nb.c
        libspeex/ltp.c
        libspeex/math_approx.c
        libspeex/mdf.c
        libspeex/medfilter.c
        libspeex/misc.c
        libspeex/modes.c
        libspeex/nb_celp.c
        libspeex/preprocess.c
        libspeex/quant_lsp.c
        libspeex/sb_celp.c
        libspeex/smallft.c
        libspeex/speex.c
        libspeex/speex_callbacks.c
        libspeex/speex_header.c
        libspeex/stereo.c
        libspeex/vbr.c
        libspeex/vq.c
        )

    target_include_directories(iaxclient_lib PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/iaxclient/lib/libspeex/include)
endif()

# never used for the moment
# if (ENABLE_ALSA)
#     target_sources(iaxclient_lib PRIVATE audio_alsa.c)
# endif(ENABLE_ALSA)

# if (ENABLE_PORTAUDIO)
#     target_sources(iaxclient_lib PRIVATE audio_portaudio.c)
# endif(ENABLE_PORTAUDIO)

# eof
