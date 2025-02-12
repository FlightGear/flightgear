# helpers around OSG locations versions

########################################################################################
# OSG plugin detection

if (MSVC)
    set(OSG_PLUGIN_SUFFIX "bin")
else()
    set(OSG_PLUGIN_SUFFIX "lib" "lib64")

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