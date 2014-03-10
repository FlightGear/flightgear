
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/hts_engine_API/include)

set(HTS_ENGINE_SOURCES
    lib/HTS_audio.c
    lib/HTS_engine.c
    lib/HTS_gstream.c
    lib/HTS_label.c
    lib/HTS_misc.c
    lib/HTS_model.c
    lib/HTS_pstream.c
    lib/HTS_sstream.c
    lib/HTS_vocoder.c
)
    

add_library(hts_engine STATIC 
    ${HTS_ENGINE_SOURCES}
)
