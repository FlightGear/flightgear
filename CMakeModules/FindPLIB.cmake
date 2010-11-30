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

FIND_PATH(PLIB_INCLUDE_DIR ul.h
  HINTS $ENV{PLIBDIR}
  PATH_SUFFIXES include/plib include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

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

macro(find_static_component comp libs)
	set(compLib "plib${comp}")
	string(TOUPPER "PLIB_${comp}_LIBRARY" compLibName)
	
	FIND_LIBRARY(${compLibName}
	  NAMES ${compLib}
	  HINTS $ENV{PLIBDIR}
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
		#set(PLIB_LIBRARIES "${PLIB_LIBRARIES} ${componentLib}" PARENT_SCOPE)
	endif()
endmacro()

if(${PLIB_LIBRARIES} STREQUAL "PLIB_LIBRARIES-NOTFOUND")	
	set(PLIB_LIBRARIES "") # clear value
	
# based on the contents of deps, add other required PLIB
# static library dependencies. Eg PUI requires SSG and FNT
	set(outDeps ${PLIB_FIND_COMPONENTS})
	
	foreach(c ${PLIB_FIND_COMPONENTS})
		if (${c} STREQUAL "pu")
			list(APPEND outDeps "fnt" "ssg" "sg")
		elseif (${c} STREQUAL "puaux")
			list(APPEND outDeps "pu" "fnt" "ssg" "sg")
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
	elseif(WINDOWS)
		find_library(WINMM_LIBRARY winmm)
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

