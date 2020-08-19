
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

if (ENABLE_IAX)
    add_subdirectory(iaxclient/lib)
endif()

add_subdirectory(mongoose)
add_subdirectory(cjson)

add_subdirectory(flite_hts_engine)

if (NOT SYSTEM_HTS_ENGINE)
    add_subdirectory(hts_engine_API)
endif()

if (ENABLE_HID_INPUT)
    add_subdirectory(hidapi)
endif()

add_subdirectory(fonts)

if (ENABLE_PLIB_JOYSTICK)
    add_subdirectory(joystick)
endif()

if (NOT SYSTEM_CPPUNIT)
    add_subdirectory(cppunit)
endif()
