function(setup_fgfs_libraries target)
    link_directories ( ${Boost_LIBRARY_DIRS} )

    get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
    #message(STATUS "fg libs ${FG_LIBS}")
    #message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
    #message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

    if(RTI_FOUND)
        set(HLA_LIBRARIES ${RTI_LDFLAGS})
    else()
        set(HLA_LIBRARIES "")
    endif()

    if(GDAL_FOUND)
        set(GDAL_LIBRARIES ${GDAL_LIBRARY})
    else()
        set(GDAL_LIBRARIES "")
    endif()

    if(ENABLE_JSBSIM)
        target_link_libraries(${target} JSBSim)
    endif()

    if(ENABLE_IAX)
        target_link_libraries(${target} iaxclient_lib ${OPENAL_LIBRARY})
    endif()

    # manually created DBus target
    if(TARGET DBus)
        target_link_libraries(${target} DBus)
    endif()

    # PkgConfig::DBUS target

    if(CMAKE_VERSION VERSION_LESS 3.6)
        if(DBUS_FOUND)
            target_link_libraries(${target} ${DBUS_LDFLAGS})
        endif()
    else()
        # PkgConfig::DBUS target
        if(TARGET PkgConfig::DBUS)
            target_link_libraries(${target} PkgConfig::DBUS)
        endif()
    endif()

    if(FG_HAVE_GPERFTOOLS)
        target_link_libraries(${target} ${GooglePerfTools_LIBRARIES})
    endif()

    if(X11_FOUND)
        target_link_libraries(${target} ${X11_LIBRARIES})
    endif()

    target_link_libraries(${target}
        SimGearCore
        SimGearScene
        ${EVENT_INPUT_LIBRARIES}
        ${GDAL_LIBRARIES}
        ${HLA_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${PLATFORM_LIBS}
        ${PLIB_LIBRARIES}
        ${SQLITE3_LIBRARY}
        ${SIMGEAR_LIBRARIES}
    )

    if (ENABLE_PLIB_JOYSTICK)
        target_link_libraries(${target} PLIBJoystick)
    endif()

    target_link_libraries(${target} PLIBFont)

    if(SYSTEM_HTS_ENGINE)
        target_link_libraries(${target} flite_hts ${HTS_ENGINE_LIBRARIES})
    else()
        target_link_libraries(${target} flite_hts hts_engine)
    endif()

    if(Qt5Core_FOUND)
        target_link_libraries(${target} Qt5::Core Qt5::Widgets fglauncher fgqmlui)
        set_property(TARGET ${target} PROPERTY AUTOMOC ON)
    endif()

    if(USE_AEONWAVE)
       target_link_libraries(${target} ${AAX_LIBRARY})
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(${target} execinfo)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
        target_link_libraries(${target} execinfo)
    endif()
endfunction()
