
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

target_include_directories(hts_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(Flightgear::HTSEngine ALIAS hts_engine)
