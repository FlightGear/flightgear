
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

if (ENABLE_IAX)
    add_subdirectory(iaxclient/lib)
endif()

add_subdirectory(mongoose)
add_subdirectory(cjson)

if (ENABLE_FLITE)
    add_subdirectory(hts_engine_API)
    add_subdirectory(flite_hts_engine)
endif()
