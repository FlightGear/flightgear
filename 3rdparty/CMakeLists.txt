
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

if (ENABLE_IAX)
    add_subdirectory(iaxclient/lib)
endif()
