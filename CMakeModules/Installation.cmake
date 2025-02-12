

if (TARGET sentry_crashpad::handler)
    if (APPLE)
        # install inside the bundle
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION fgfs.app/Contents/MacOS OPTIONAL)
    else()
        # install in the bin-dir, next to the application binary
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    endif()
endif()

if (HAVE_QT)
    include (QtDeployment)
endif()



find_package(Git)

if (MSVC)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/generateInnoSetupConfig.cmake.in 
        ${CMAKE_BINARY_DIR}/generateInnoSetupConfig.cmake
        @ONLY)

    install(SCRIPT ${CMAKE_BINARY_DIR}/generateInnoSetupConfig.cmake COMPONENT packaging )

    # important we use install() here so that passing a custom prefix to
    # 'cmake --install --prefix FOO' works correctly to put the file somewhere special
    install(FILES ${CMAKE_BINARY_DIR}/InstallConfig.iss DESTINATION . COMPONENT packaging )
endif()


########################################################################################

# OSG libs
foreach (osglib OSG OpenThreads osgUtil osgText osgGA osgSim osgParticle osgTerrain osgViewer osgDB)
    if (APPLE)
        install(FILES
                $<TARGET_FILE:OSG::${osglib}>  
            DESTINATION 
                $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks
        )
    endif()
endforeach()

if (APPLE)
    # OSG plugins
    install(DIRECTORY ${OSG_PLUGINS_DIR} DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/PlugIns)

    # add extra utilites to the bundle
    install(TARGETS fgcom fgjs fgelev DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/MacOS)

    if (TARGET sentry::sentry)
        install(FILES $<TARGET_FILE:sentry::sentry> DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks)
    endif()

    if (TARGET sentry_crashpad::handler)
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/MacOS)
    endif()

    if (TARGET DBus::DBus)
        #get_target_property(dbusLib DBus::DBus IMPORTED_LOCATION)
        #message(STATUS "DBus library at: ${dbusLib}")
        #install(FILES ${dbusLib} DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/MacOS)
        install(FILES $<TARGET_FILE:DBus::DBus> DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks)
    endif()

    # FIXME: this copies the fully version file name, need to rename to the non-versioned one
    install(FILES 
            $<TARGET_FILE:OpenAL::OpenAL>  
        DESTINATION 
            $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks
    )
    
    install(FILES ${CMAKE_SOURCE_DIR}/package/mac/FlightGear.icns DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Resources)
endif()
 
########################################################################################
# AppDir creation for Linux AppImage

if (LINUX)
    
    install(TARGETS fgcom fgjs fgelev fgfs 
        DESTINATION appdir/usr/bin 
        COMPONENT packaging EXCLUDE_FROM_ALL)

    install(DIRECTORY ${OSG_PLUGINS_DIR} 
        DESTINATION appdir/usr/lib 
        COMPONENT packaging EXCLUDE_FROM_ALL)

    install(FILES /etc/ssl/certs/ca-certificates.crt
	DESTINATION appdir/usr/ssl
	RENAME cacert.pem
	OPTIONAL
	COMPONENT packaging EXCLUDE_FROM_ALL)
    install(CODE "
	if(NOT EXISTS /etc/ssl/certs/ca-certificates.crt)
            message(WARNING \"No SSL certificates found, will not be included in AppImage\")
	endif()
    ")
    # TODO: things under share/
endif()


########################################################################################
# actual app installation: this needs to happen late, after the various TARGET_BUNDLE_CONTENT_DIR
# rules are applied

if (APPLE)
    install(TARGETS fgfs BUNDLE DESTINATION .)
else()
    install(TARGETS fgfs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()


#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
    "${PROJECT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

if (NOT TARGET uninstall)
    ADD_CUSTOM_TARGET(uninstall
        "${CMAKE_COMMAND}" -P "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

