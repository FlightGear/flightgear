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

    if(ENABLE_OSGXR)
        target_link_libraries(${target} osgXR::osgXR)
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


# CMake < 3.12 can't define a link to an OBJECT library to specify its include
# paths, so we have to essentially duplicate the above and configure the paths manually.
# Once we require CMake 3.12, delete all this and use the function above.

function (_apply_target_includes dest target)
    if (NOT TARGET ${target})
        return()
    endif()

    # retrieve the list of includes from the target
    get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)

    foreach(i ${includes})
        # skip any invalid includes
        if (NOT i)
            return()
        endif()

        target_include_directories(${dest} PUBLIC ${i})
    endforeach()
endfunction()

function (_apply_all_target_includes dest)
    foreach(arg IN LISTS ARGN)
      _apply_target_includes(${dest} ${arg})
    endforeach()
endfunction()

function (setup_fgfs_library_includes target)
 
    _apply_all_target_includes(${target} 
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

    if(ENABLE_JSBSIM)
        _apply_target_includes(${target} JSBSim)
    endif()    
    
    _apply_target_includes(${target} fgsqlite3)
    _apply_target_includes(${target} fgvoicesynth)
    _apply_target_includes(${target} fgembeddedresources)

    if(ENABLE_IAX)
        _apply_target_includes(${target} iaxclient_lib)
    endif()

    if (ENABLE_SWIFT)
        _apply_all_target_includes(${target} ${dbus_target} ${libEvent_target})
    elseif(HAVE_DBUS)
        _apply_all_target_includes(${target} ${dbus_target})
    endif()

    if (ENABLE_PLIB_JOYSTICK)
        _apply_target_includes(${target} PLIBJoystick)
    endif()
    
    _apply_target_includes(${target} PLIBFont)

    if (TARGET sentry::sentry)
        _apply_target_includes(${target} sentry::sentry)
    endif()

endfunction()