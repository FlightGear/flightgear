# Locate SimGear
# This module defines
# SIMGEAR_LIBRARIES
# SIMGEAR_FOUND, if false, do not try to link to SimGear 
# SIMGEAR_INCLUDE_DIR, where to find the headers
#
# $SIMGEARDIR is an environment variable that would
# correspond to the ./configure --prefix=$SIMGEARDIR
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
# ~/Library/Frameworks/OpenAL.framework/Headers
# /Library/Frameworks/OpenAL.framework/Headers
# /System/Library/Frameworks/OpenAL.framework/Headers
#
# On OS X, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# OPENAL_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

FIND_PATH(SIMGEAR_INCLUDE_DIR simgear/math/SGMath.hxx
  HINTS $ENV{SIMGEARDIR}
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
  $ENV{SIMGEARDIR}
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
	string(TOUPPER "SIMGEAR_${comp}_LIBRARY" compLibName)
	
	FIND_LIBRARY(${compLibName}
	  NAMES ${compLib}
	  HINTS $ENV{SIMGEARDIR}
	  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	  PATHS
	  /usr/local
	  /usr
	  /opt
	)
	
	set(componentLib ${${compLibName}})
	if (NOT ${componentLib} STREQUAL "componentLib-NOTFOUND")
		#message(STATUS "found ${componentLib}")
		list(APPEND ${libs} ${componentLib})
	endif()
endmacro()

if(${SIMGEAR_LIBRARIES} STREQUAL "SIMGEAR_LIBRARIES-NOTFOUND")	
	set(SIMGEAR_LIBRARIES "") # clear value
	
	set(comps 
		ephemeris
		bucket
		serial
		screen
		environment
		nasal
		props
		xml
		debug math bvh 
		model material tgdb
		sky
		structure
		util route
		timing
		threads
		io
		sound
		misc
		magvar)
	
	foreach(component ${comps})
		find_sg_component(${component} SIMGEAR_LIBRARIES)
	endforeach()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SimGear DEFAULT_MSG SIMGEAR_LIBRARIES SIMGEAR_INCLUDE_DIR)

