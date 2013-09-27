
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

add_subdirectory(iaxclient/lib)
