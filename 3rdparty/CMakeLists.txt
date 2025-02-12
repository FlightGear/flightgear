
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

if (ENABLE_PLIB_JOYSTICK)
    add_subdirectory(joystick)
endif()

if (NOT SYSTEM_CPPUNIT)
    add_subdirectory(cppunit)
endif()

if (ENABLE_OSGXR AND NOT ENABLE_SYSTEM_OSGXR)

    # Core profile: generated osg/GL includes <GL/glcorearb.h>, but we
    # install that as part of SimGear. So we need to extend this value
    # so osgXR can build
    if (WIN32)
        get_target_property(simgearScene_includes SimGearScene INTERFACE_INCLUDE_DIRECTORIES)
        list(APPEND OPENSCENEGRAPH_INCLUDE_DIRS "${simgearScene_includes}")
    endif()

    add_subdirectory(osgXR)
endif()
