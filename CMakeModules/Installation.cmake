

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


if (MSVC)
    set(OSG_PLUGIN_SUFFIX "bin")
else()
    set(OSG_PLUGIN_SUFFIX "lib")
endif()

find_path(OSG_PLUGINS_DIR
    NAMES osgPlugins 
        osgPlugins-${OPENSCENEGRAPH_VERSION}
    PATHS 
        ${FINAL_MSVC_3RDPARTY_DIR}
    PATH_SUFFIXES
        ${OSG_PLUGIN_SUFFIX}
)

if (NOT OSG_PLUGINS_DIR)
    message(FATAL_ERROR "Couldn't find osgPlugins directory")
endif()

message(STATUS "OSG plugins at: ${OSG_PLUGINS_DIR}/osgPlugins")

get_filename_component(OSG_BASE_DIR ${OSG_PLUGINS_DIR} DIRECTORY)


#if (MSVC)
    configure_file(${CMAKE_SOURCE_DIR}/package/windows/BuildConfig.iss.in ${CMAKE_BINARY_DIR}/BuildConfig.iss)
    install(FILES ${CMAKE_BINARY_DIR}/BuildConfig.iss DESTINATION .)
#endif()

if (APPLE)
    # OSG libs

    # OSG plugins
    install(DIRECTORY ${OSG_PLUGINS_DIR}/osgPlugins DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/PlugIns)

    # add extra utilites to the bundle
    install(TARGETS fgcom fgjs fgelev DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/MacOS)

    # FIXME: this copies the fully version file name, need to rename to the non-versioned one
    install(FILES 
            $<TARGET_FILE:OpenAL::OpenAL>  
            $<TARGET_FILE:LibLZMA::LibLZMA> 
        DESTINATION 
            $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks
    )
    
    install(FILES ${CMAKE_SOURCE_DIR}/package/mac/FlightGear.icns DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Resources)
endif()


 

#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
    "${PROJECT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake")

