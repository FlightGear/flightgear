function(setup_fgfs_libraries target)
    get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
    #message(STATUS "fg libs ${FG_LIBS}")
    #message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
    #message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

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

    if(TARGET DBus)
        target_link_libraries(${target} DBus)
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

    if (ENABLE_PLIB_JOYSTICK)
        target_link_libraries(${target} PLIBJoystick)
    endif()

    target_link_libraries(${target} PLIBFont)

    if (TARGET fglauncher)
        target_link_libraries(${target} Qt5::Core Qt5::Widgets fglauncher fgqmlui)
        set_property(TARGET ${target} PROPERTY AUTOMOC ON)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(${target} execinfo)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
        target_link_libraries(${target} execinfo)
    endif()
endfunction()
