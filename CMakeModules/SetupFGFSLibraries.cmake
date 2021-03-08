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
        # ALIAS doesn't work with CMake 3.10, so we need
        # variable to store the target name
        target_link_libraries(${target} ${dbus_target})
    endif()

    if(X11_FOUND)
        target_link_libraries(${target} ${X11_LIBRARIES})
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
        ${PLIB_LIBRARIES}
    )

    if (ENABLE_SWIFT)
        # ALIAS doesn't work with CMake 3.10, so we need
        # variable to store the target name
        target_link_libraries(${target} ${dbus_target} ${libEvent_target})
    endif()

    if (ENABLE_PLIB_JOYSTICK)
        target_link_libraries(${target} PLIBJoystick)
    endif()

    target_link_libraries(${target} PLIBFont)

    # FIXME : rewrite options.cxx / SetupRootDialog.hxx not
    # to require this dependency
    if (HAVE_QT)
        target_link_libraries(${target} Qt5::Core Qt5::Widgets fglauncher fgqmlui)
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
