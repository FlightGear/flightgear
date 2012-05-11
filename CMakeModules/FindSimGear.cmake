# Locate SimGear
# This module defines

# SIMGEAR_CORE_LIBRARIES, a list of the core static libraries
# SIMGEAR_LIBRARIES, a list of all the static libraries (core + scene)
# SIMGEAR_FOUND, if false, do not try to link to SimGear
# SIMGEAR_INCLUDE_DIR, where to find the headers
#
# $SIMGEAR_DIR is an environment variable that would
# correspond to the ./configure --prefix=$SIMGEAR_DIR
# used in building SimGear.
#
# Created by James Turner. This was influenced by the FindOpenAL.cmake module.

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

include(SelectLibraryConfigurations)

macro(find_sg_library libName varName libs)
    set(libVarName "${varName}_LIBRARY")
    # do not cache the library check
    unset(${libVarName}_DEBUG CACHE)
    unset(${libVarName}_RELEASE CACHE)

    FIND_LIBRARY(${libVarName}_DEBUG
      NAMES ${libName}${CMAKE_DEBUG_POSTFIX}
      HINTS $ENV{SIMGEAR_DIR}
      PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR} libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
    )
    FIND_LIBRARY(${libVarName}_RELEASE
      NAMES ${libName}${CMAKE_RELEASE_POSTFIX}
      HINTS $ENV{SIMGEAR_DIR}
      PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR} libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
    )
    
   # message(STATUS "before: Simgear ${${libVarName}_RELEASE} ")
  #  message(STATUS "before: Simgear ${${libVarName}_DEBUG} ")
    
    select_library_configurations( ${varName} )

  #  message(STATUS "after:Simgear ${${libVarName}_RELEASE} ")
  #  message(STATUS "after:Simgear ${${libVarName}_DEBUG} ")

    set(componentLibRelease ${${libVarName}_RELEASE})
  #  message(STATUS "Simgear ${libVarName}_RELEASE ${componentLibRelease}")
    set(componentLibDebug ${${libVarName}_DEBUG})
   # message(STATUS "Simgear ${libVarName}_DEBUG ${componentLibDebug}")
    
    if (NOT ${libVarName}_DEBUG)
        if (NOT ${libVarName}_RELEASE)
            #message(STATUS "found ${componentLib}")
            list(APPEND ${libs} ${componentLibRelease})
        endif()
    else()
        list(APPEND ${libs} optimized ${componentLibRelease} debug ${componentLibDebug})
    endif()
endmacro()

macro(find_sg_component comp libs)
    set(compLib "sg${comp}")
    string(TOUPPER "SIMGEAR_${comp}" libVar)
    
    find_sg_library(${compLib} ${libVar} ${libs})
endmacro()

FIND_PATH(SIMGEAR_INCLUDE_DIR simgear/math/SGMath.hxx
  HINTS $ENV{SIMGEAR_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

# make sure the simgear include directory exists
if (NOT SIMGEAR_INCLUDE_DIR)
    message(FATAL_ERROR "Cannot find SimGear includes! (Forgot 'make install' for SimGear?) "
            "Compile & INSTALL SimGear before configuring FlightGear. "
            "When using non-standard locations, use 'SIMGEAR_DIR' to configure the SimGear location.")
endif()

message(STATUS "SimGear include directory: ${SIMGEAR_INCLUDE_DIR}")

# make sure the simgear/version.h header exists
if (NOT EXISTS ${SIMGEAR_INCLUDE_DIR}/simgear/version.h)
    message(FATAL_ERROR "Found SimGear, but it does not contain a simgear/version.h include! "
            "SimGear installation is incomplete.")
endif()

# read the simgear version header file, get the version
file(READ ${SIMGEAR_INCLUDE_DIR}/simgear/version.h sgVersionFile)
string(STRIP ${sgVersionFile} SIMGEAR_DEFINE)
string(REPLACE "#define SIMGEAR_VERSION " "" SIMGEAR_VERSION ${SIMGEAR_DEFINE})
message(STATUS "found SimGear version: ${SIMGEAR_VERSION} (needed ${SimGear_FIND_VERSION})")

if(NOT ${SIMGEAR_VERSION} EQUAL ${SimGear_FIND_VERSION})
    message(FATAL_ERROR "You have installed a mismatching SimGear version ${SIMGEAR_VERSION} "
            "instead of ${SimGear_FIND_VERSION} as required by FlightGear. "
            "When using multiple SimGear installations, please use 'SIMGEAR_DIR' "
            "to select the SimGear library location to be used.")
endif()

# dependent packages
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if(SIMGEAR_SHARED)
    message(STATUS "looking for shared Simgear libraries")

    find_sg_library(SimGearCore SIMGEAR_CORE SIMGEAR_CORE_LIBRARIES)
    find_sg_library(SimGearScene SIMGEAR_SCENE SIMGEAR_LIBRARIES)

 
    list(APPEND SIMGEAR_LIBRARIES ${SIMGEAR_CORE_LIBRARIES})
    set(SIMGEAR_CORE_LIBRARY_DEPENDENCIES "")
    set(SIMGEAR_SCENE_LIBRARY_DEPENDENCIES "")
    
   # message(STATUS "core lib ${SIMGEAR_CORE_LIBRARIES}")
  #  message(STATUS "all libs ${SIMGEAR_LIBRARIES}")
else(SIMGEAR_SHARED)

    set(SIMGEAR_LIBRARIES "") # clear value
    set(SIMGEAR_CORE_LIBRARIES "") # clear value
    message(STATUS "looking for static SimGear libraries")
    
  # note the order here affects the order Simgear libraries are
  # linked in, and hence ability to link when using a traditional
  # linker such as GNU ld on Linux
    set(comps
        environment
        nasal
        tsync
        bucket
        io
        serial
        math
        props
        structure
        timing
        xml
        misc
        threads
        debug
        magvar
    )

    set(scene_comps
        ephem
        sky
        material
        tgdb
        model
        screen
        bvh
        util
        sound)
            
    foreach(component ${comps})
        find_sg_component(${component} SIMGEAR_CORE_LIBRARIES)
    endforeach()

    foreach(component ${scene_comps})
        find_sg_component(${component} SIMGEAR_LIBRARIES)
    endforeach()

    # again link order matters - scene libraries depend on core ones
    list(APPEND SIMGEAR_LIBRARIES ${SIMGEAR_CORE_LIBRARIES})

    #message(STATUS "all libs ${SIMGEAR_LIBRARIES}")
    
    set(SIMGEAR_CORE_LIBRARY_DEPENDENCIES
        ${CMAKE_THREAD_LIBS_INIT}
        ${ZLIB_LIBRARY}
        ${LIBSVN_LIBRARIES}
        ${WINMM_LIBRARY})

    set(SIMGEAR_SCENE_LIBRARY_DEPENDENCIES 
        ${ALUT_LIBRARY} 
        ${OPENAL_LIBRARY})

    if(WIN32)
        list(APPEND SIMGEAR_CORE_LIBRARY_DEPENDENCIES ws2_32.lib)
    endif(WIN32)

    if(NOT MSVC)
        # basic timing routines on non windows systems, may be also cygwin?!
        check_library_exists(rt clock_gettime "" have_rt)
        if(have_rt)
            list(APPEND SIMGEAR_CORE_LIBRARY_DEPENDENCIES rt)
        endif(have_rt)
    endif(NOT MSVC)
endif(SIMGEAR_SHARED)

if((NOT SIMGEAR_CORE_LIBRARIES)OR(NOT SIMGEAR_LIBRARIES))
    message(FATAL_ERROR "Cannot find SimGear libraries! (Forgot 'make install' for SimGear?) "
            "Compile & INSTALL SimGear before configuring FlightGear. "
            "When using non-standard locations, use 'SIMGEAR_DIR' to configure the SimGear location.")
else()
    message(STATUS "found SimGear libraries")
endif()

# now we've found SimGear, try test-compiling using its includes
include(CheckCXXSourceRuns)

SET(CMAKE_REQUIRED_INCLUDES ${SIMGEAR_INCLUDE_DIR})

# clear cache, run a fresh compile test every time
unset(SIMGEAR_COMPILE_TEST CACHE)

# disable OSG dependencies for test-compiling
set(CMAKE_REQUIRED_DEFINITIONS "-DNO_OPENSCENEGRAPH_INTERFACE")
check_cxx_source_runs(
    "#include <cstdio>
    #include \"simgear/version.h\"
    #include \"simgear/math/SGMath.hxx\"

    #define xstr(s) str(s)
    #define str(s) #s

    #define MIN_MAJOR ${SimGear_FIND_VERSION_MAJOR}
    #define MIN_MINOR ${SimGear_FIND_VERSION_MINOR}
    #define MIN_MICRO ${SimGear_FIND_VERSION_PATCH}

    int main() {
        int major, minor, micro;

        /* printf(%d.%d.%d or greater, , MIN_MAJOR, MIN_MINOR, MIN_MICRO); */
        printf(\"found %s ... \", xstr(SIMGEAR_VERSION));

        sscanf( xstr(SIMGEAR_VERSION), \"%d.%d.%d\", &major, &minor, &micro );

        if ( (major != MIN_MAJOR) ||
             (minor != MIN_MINOR) ||
             (micro != MIN_MICRO) ) {
         return -1;
        }

        return 0;
    }
    "
    SIMGEAR_COMPILE_TEST)

if(NOT SIMGEAR_COMPILE_TEST)
    message(FATAL_ERROR "Oops, you have installed SimGear includes, however test compiling failed. "
            "Try removing 'CMakeCache.txt' and reconfigure with 'cmake'.")
endif()
unset(CMAKE_REQUIRED_DEFINITIONS)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SimGear DEFAULT_MSG
     SIMGEAR_LIBRARIES SIMGEAR_CORE_LIBRARIES SIMGEAR_INCLUDE_DIR SIMGEAR_COMPILE_TEST)

