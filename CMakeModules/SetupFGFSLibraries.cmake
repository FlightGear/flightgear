function(setup_fgfs_libraries target)
    if(RTI_FOUND)
        set(HLA_LIBRARIES ${RTI_LDFLAGS})
    else()
        set(HLA_LIBRARIES "")
    endif()

    if(ENABLE_JSBSIM)
        target_link_libraries(${target} JSBSim)
    endif()

    if(ENABLE_IAX)
        target_link_libraries(${target} iaxclient_lib)
    endif()

    if(HAVE_DBUS)
        target_link_libraries(${target} DBus::DBus)
    endif()

    if(X11_FOUND)
        target_link_libraries(${target} ${X11_LIBRARIES})
    endif()

    if(ENABLE_OSGXR)
        target_link_libraries(${target} osgXR)
    endif()

    target_link_libraries(${target} fgsqlite3 fgvoicesynth fgembeddedresources)

    target_link_libraries(${target}
        SimGearCore
        SimGearScene
        Boost::boost
        ${EVENT_INPUT_LIBRARIES}
        ${HLA_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${PLATFORM_LIBS}
    )

    if (ENABLE_SWIFT)
        target_link_libraries(${target} DBus::DBus libEvent::libEvent)
    endif()

    if (ENABLE_PLIB_JOYSTICK)
        target_link_libraries(${target} PLIBJoystick)
    endif()

    if (HAVE_QT)
        target_link_libraries(${target} fglauncher fgqmlui)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(${target} execinfo)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
        target_link_libraries(${target} execinfo)
    endif()

    if (TARGET sentry::sentry)
        target_link_libraries(${target} sentry::sentry)
    endif()
endfunction()

