

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

########################################################################################
# OSG plugin detection

if (MSVC)
    set(OSG_PLUGIN_SUFFIX "bin")
else()
    set(OSG_PLUGIN_SUFFIX "lib")

    # needed for Debian where the plugins might be at /usr/lib/x86_64-linux-gnu
    if (CMAKE_LIBRARY_ARCHITECTURE) 
        list(APPEND OSG_PLUGIN_SUFFIX "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
    endif()
endif()

# we can't use NAMES plural because we need to know which name was found
# so instead do a manual loop
foreach(osgPluginDirName osgPlugins osgPlugins-${OPENSCENEGRAPH_VERSION})
    find_path(osgPluginLocation
        NAMES ${osgPluginDirName} 
            
        PATHS 
            ${FINAL_MSVC_3RDPARTY_DIR}
        PATH_SUFFIXES
            ${OSG_PLUGIN_SUFFIX}
    )

    if (osgPluginLocation)
        set(OSG_PLUGINS_DIR "${osgPluginLocation}/${osgPluginDirName}")
        get_filename_component(OSG_BASE_DIR ${osgPluginLocation} DIRECTORY)
        break()
    endif()
endforeach()

if (NOT OSG_PLUGINS_DIR)
    message(FATAL_ERROR "Couldn't find osgPlugins directory")
endif()

message(STATUS "OSG plugins at: ${OSG_PLUGINS_DIR}")

########################################################################################
# find OpenThreads and OpenSceneGraph DLL versions
# would be simpler with CMake 3.29 where file(STRINGS) can do regular expression matching
# directly, but this is not too bad at least

set(_osg_Version_file "${OSG_INCLUDE_DIR}/osg/Version")
set(_openthreads_Version_file "${OSG_INCLUDE_DIR}/OpenThreads/Version")
if( NOT EXISTS "${_osg_Version_file}" OR NOT EXISTS ${_openthreads_Version_file})
    message(FATAL_ERROR "Couldn't find OpenSceneGraph or OpenThreads Version headers.")
endif()

file(STRINGS "${_osg_Version_file}" _osg_Version_contents
        REGEX "#define OPENSCENEGRAPH_SOVERSION[ \t]+[0-9]+")

file(STRINGS "${_openthreads_Version_file}" _openthreads_Version_contents
        REGEX "#define OPENTHREADS_SOVERSION[ \t]+[0-9]+")

string(REGEX REPLACE ".*#define OPENSCENEGRAPH_SOVERSION[ \t]+([0-9]+).*"
    "\\1" osg_soversion ${_osg_Version_contents})

string(REGEX REPLACE ".*#define OPENTHREADS_SOVERSION[ \t]+([0-9]+).*"
    "\\1" openthreads_soversion ${_openthreads_Version_contents})

message(STATUS "OSG SO version: ${osg_soversion}")
message(STATUS "OpenThreads SO version: ${openthreads_soversion}")

string(TIMESTAMP iss_config_timestamp)

########################################################################################

#if (MSVC)
    # iscc.exe only accepts Windows style-paths, so explicitly convert
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" FG_WINDOWS_INSTALL_PREFIX)
    file(TO_NATIVE_PATH "${OSG_BASE_DIR}" INNO_SETUP_OSG_BASE_DIR)
    file(TO_NATIVE_PATH "${FINAL_MSVC_3RDPARTY_DIR}" INNO_SETUP_3RDPARTY_DIR)

    configure_file(${CMAKE_SOURCE_DIR}/package/windows/InstallConfig.iss.in ${CMAKE_BINARY_DIR}/InstallConfig.iss)
    install(FILES ${CMAKE_BINARY_DIR}/InstallConfig.iss 
        DESTINATION . 
        COMPONENT packaging EXCLUDE_FROM_ALL)
#endif()

# OSG libs
foreach (osglib OSG OpenThreads osgUtil osgText osgGA osgSim osgParticle osgTerrain osgViewer osgDB)
    if (APPLE)
        install(FILES
                $<TARGET_FILE:OSG::${osglib}>  
            DESTINATION 
                $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/Frameworks
        )
    endif()

    if (LINUX)
        install(FILES $<TARGET_FILE:OSG::${osglib}>  
            DESTINATION appdir/usr/lib
            COMPONENT packaging EXCLUDE_FROM_ALL)
    endif()
endforeach()

if (APPLE)
    # OSG plugins
    install(DIRECTORY ${OSG_PLUGINS_DIR} DESTINATION $<TARGET_BUNDLE_CONTENT_DIR:fgfs>/PlugIns)

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
 
########################################################################################
# AppDir creation for Linux AppImage

if (LINUX)
    
    install(DIRECTORY ${OSG_PLUGINS_DIR} 
        DESTINATION appdir/usr/lib 
        COMPONENT packaging EXCLUDE_FROM_ALL)
    install(TARGETS fgcom fgjs fgelev fgfs 
        DESTINATION appdir/usr/bin 
        COMPONENT packaging EXCLUDE_FROM_ALL)

    # TODO: things under share/
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

