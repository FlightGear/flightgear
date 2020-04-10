
if (NOT SYSTEM_SQLITE)
    add_subdirectory(sqlite3)
endif()

if (ENABLE_IAX)
    add_subdirectory(iaxclient/lib)
endif()

add_subdirectory(mongoose)
add_subdirectory(cjson)

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
