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

include(SelectLibraryConfigurations)

FIND_PATH(PLIB_INCLUDE_DIR plib/ul.h
  PATH_SUFFIXES include
  HINTS $ENV{PLIBDIR}
  PATHS ${ADDITIONAL_LIBRARY_PATHS}
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
      NAMES ${compLib}_d plib_${compLib}_d
      HINTS $ENV{PLIBDIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS ${ADDITIONAL_LIBRARY_PATHS}
    )
    FIND_LIBRARY(${compLibName}_RELEASE
      NAMES ${compLib} plib_${compLib}
      HINTS $ENV{PLIBDIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS ${ADDITIONAL_LIBRARY_PATHS}
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

# based on the contents of deps, add other required PLIB
# static library dependencies. Eg PUI requires FNT
  set(outDeps ${PLIB_FIND_COMPONENTS})

  foreach(c ${PLIB_FIND_COMPONENTS})
      if (${c} STREQUAL "pu")
          # handle MSVC confusion over pu/pui naming, by removing
          # 'pu' and then adding it back
          list(REMOVE_ITEM outDeps "pu" "fnt" "sg")
          list(APPEND outDeps ${PUNAME} "sg")
      elseif (${c} STREQUAL "puaux")
          list(APPEND outDeps ${PUNAME} "sg")
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

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLIB DEFAULT_MSG PLIB_LIBRARIES PLIB_INCLUDE_DIR)
