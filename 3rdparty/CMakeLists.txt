
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

if (ENABLE_IAX)
    add_subdirectory(iaxclient/lib)
endif()

add_subdirectory(mongoose)
add_subdirectory(cjson)

if (ENABLE_FLITE)
    add_subdirectory(flite_hts_engine)

    if (SYSTEM_HTS_ENGINE)
        set(HTS_Engine_FIND_REQUIRED TRUE)
        find_package(HtsEngine)

        include_directories(${HTS_ENGINE_INCLUDE_DIR})
        message(STATUS "Using HTS engine includes at: ${HTS_ENGINE_INCLUDE_DIR}")
        message(STATUS "Using HTS engine libraries: ${HTS_ENGINE_LIBRARIES}")
    else()
        add_subdirectory(hts_engine_API)
        include_directories(${PROJECT_SOURCE_DIR}/3rdparty/hts_engine_API/include )
    endif()
endif()
