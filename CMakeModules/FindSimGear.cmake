# Locate SimGear
# This module defines
# SIMGEAR_LIBRARIES
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

# Per my request, CMake should search for frameworks first in
# the following order:
# ~/Library/Frameworks/SimGear.framework/Headers
# /Library/Frameworks/SimGear.framework/Headers
# /System/Library/Frameworks/SimGear.framework/Headers
#
# On OS X, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# SimGear_LIBRARIES to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

include(SelectLibraryConfigurations)

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

message(STATUS ${SIMGEAR_INCLUDE_DIR})

# check for dynamic framework/library
FIND_LIBRARY(SIMGEAR_LIBRARIES
  NAMES simgear SimGear
  HINTS
  $ENV{SIMGEAR_DIR}
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

macro(find_sg_component comp libs)
    set(compLib "sg${comp}")
    string(TOUPPER "SIMGEAR_${comp}" compLibBase)
    set( compLibName ${compLibBase}_LIBRARY )

    FIND_LIBRARY(${compLibName}_DEBUG
      NAMES ${compLib}${CMAKE_DEBUG_POSTFIX}
      HINTS $ENV{SIMGEAR_DIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
    )
    FIND_LIBRARY(${compLibName}_RELEASE
      NAMES ${compLib}${CMAKE_RELEASE_POSTFIX}
      HINTS $ENV{SIMGEAR_DIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
    )
    select_library_configurations( ${compLibBase} )

    set(componentLibRelease ${${compLibName}_RELEASE})
    #message(STATUS "Simgear ${compLibName}_RELEASE ${componentLibRelease}")
    set(componentLibDebug ${${compLibName}_DEBUG})
    #message(STATUS "Simgear ${compLibName}_DEBUG ${componentLibDebug}")
    if (NOT ${compLibName}_DEBUG)
        if (NOT ${compLibName}_RELEASE)
            #message(STATUS "found ${componentLib}")
            list(APPEND ${libs} ${componentLibRelease})
        endif()
    else()
        list(APPEND ${libs} optimized ${componentLibRelease} debug ${componentLibDebug})
    endif()
endmacro()


if(${SIMGEAR_LIBRARIES} STREQUAL "SIMGEAR_LIBRARIES-NOTFOUND")  
    set(SIMGEAR_LIBRARIES "") # clear value
    
    if(NOT MSVC)
        # Olaf indicates that linking the threads libs causes failures
        # on MSVC builds
        set(thread_lib threads)
    endif(NOT MSVC)

  # note the order here affects the order Simgear libraries are
  # linked in, and hence ability to link when using a traditional
  # linker such as GNU ld on Linux
    set(comps 
        ephem
        tsync
        environment
        nasal
        sky
        material
        tgdb
        model    
        screen
        bucket
        bvh
        util route
        timing
        ${thread_lib}
        io
        serial
        sound
        structure
        props
        xml
        misc
        debug 
        magvar
        math)
    
    foreach(component ${comps})
        find_sg_component(${component} SIMGEAR_LIBRARIES)
    endforeach()
endif()

# now we've found SimGear, check its version

include(CheckCXXSourceRuns)

message(STATUS "looking for version: ${SimGear_FIND_VERSION}")

SET(CMAKE_REQUIRED_INCLUDES ${SIMGEAR_INCLUDE_DIR})

check_cxx_source_runs(
    "#include <cstdio>
    #include \"simgear/version.h\"
    
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

        if ( (major < MIN_MAJOR) ||
             (major == MIN_MAJOR && minor < MIN_MINOR) ||
             (major == MIN_MAJOR && minor == MIN_MINOR && micro < MIN_MICRO) ) {
         return -1;
        }

        return 0;
    }
    "
    SIMGEAR_VERSION_OK)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SimGear DEFAULT_MSG 
     SIMGEAR_LIBRARIES SIMGEAR_INCLUDE_DIR SIMGEAR_VERSION_OK)

