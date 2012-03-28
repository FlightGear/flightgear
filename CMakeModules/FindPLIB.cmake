# Locate PLIB
# This module defines
# PLIB_LIBRARIES
# PLIB_FOUND, if false, do not try to link to PLIB 
# PLIB_INCLUDE_DIR, where to find the headers
#
# $PLIBDIR is an environment variable that would
# correspond to the ./configure --prefix=$PLIBDIR
# used in building PLIB.
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
# ~/Library/Frameworks/OpenAL.framework/Headers
# /Library/Frameworks/OpenAL.framework/Headers
# /System/Library/Frameworks/OpenAL.framework/Headers
#
# On OS X, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# OPENAL_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

include(SelectLibraryConfigurations)

set(save_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
set(CMAKE_FIND_FRAMEWORK ONLY)
FIND_PATH(PLIB_INCLUDE_DIR ul.h
  PATH_SUFFIXES include/plib include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
)
set(CMAKE_FIND_FRAMEWORK ${save_FIND_FRAMEWORK})

if(NOT PLIB_INCLUDE_DIR)
    FIND_PATH(PLIB_INCLUDE_DIR plib/ul.h
      PATH_SUFFIXES include 
      HINTS $ENV{PLIBDIR}
      PATHS
      /usr/local
      /opt/local
      /usr
    )
endif()

message(STATUS ${PLIB_INCLUDE_DIR})

# check for dynamic framework on Mac ()
FIND_LIBRARY(PLIB_LIBRARIES
  NAMES plib PLIB
  HINTS
  $ENV{PLIBDIR}
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
)

if (MSVC) 
   set (PUNAME "pui")
else (MSVC)
   set (PUNAME "pu")
endif (MSVC)


macro(find_static_component comp libs)
    # account for alternative Windows PLIB distribution naming
    if(MSVC)
      set(compLib "${comp}")
    else(MSVC)
      set(compLib "plib${comp}")
    endif(MSVC)
    
    string(TOUPPER "PLIB_${comp}" compLibBase)
    set( compLibName ${compLibBase}_LIBRARY )

    FIND_LIBRARY(${compLibName}_DEBUG
      NAMES ${compLib}_d
      HINTS $ENV{PLIBDIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
    )
    FIND_LIBRARY(${compLibName}_RELEASE
      NAMES ${compLib}
      HINTS $ENV{PLIBDIR}
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

if(${PLIB_LIBRARIES} STREQUAL "PLIB_LIBRARIES-NOTFOUND")    
    set(PLIB_LIBRARIES "") # clear value
    
# based on the contents of deps, add other required PLIB
# static library dependencies. Eg PUI requires FNT
    set(outDeps ${PLIB_FIND_COMPONENTS})
    
    foreach(c ${PLIB_FIND_COMPONENTS})
        if (${c} STREQUAL "pu")
            # handle MSVC confusion over pu/pui naming, by removing
            # 'pu' and then adding it back
            list(REMOVE_ITEM outDeps "pu")
            list(APPEND outDeps ${PUNAME} "fnt" "sg")
        elseif (${c} STREQUAL "puaux")
            list(APPEND outDeps ${PUNAME} "fnt" "sg")
        elseif (${c} STREQUAL "ssg")
            list(APPEND outDeps "sg")
        endif()
    endforeach()
        
    list(APPEND outDeps "ul") # everything needs ul
    list(REMOVE_DUPLICATES outDeps) # clean up

    

    # look for traditional static libraries
    foreach(component ${outDeps})
        find_static_component(${component} PLIB_LIBRARIES)
    endforeach()
endif()

list(FIND outDeps "js" haveJs)
if(${haveJs} GREATER -1)
    message(STATUS "adding runtime JS dependencies")
    if(APPLE)
    # resolve frameworks to full paths
        find_library(IOKIT_LIBRARY IOKit)
        find_library(CF_LIBRARY CoreFoundation)
        set(JS_LIBS ${IOKIT_LIBRARY} ${CF_LIBRARY})
    elseif(WIN32)
        set(WINMM_LIBRARY winmm)
        set(JS_LIBS ${WINMM_LIBRARY})
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        # anything needed here?
    elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        find_library(USBHID_LIBRARY usbhid)
        # check_function_exists(hidinit)
        set(JS_LIBS ${USBHID_LIBRARY})
    else()
        message(WARNING "Unsupported platform for PLIB JS libs")
    endif()
    
    list(APPEND PLIB_LIBRARIES ${JS_LIBS})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLIB DEFAULT_MSG PLIB_LIBRARIES PLIB_INCLUDE_DIR)

