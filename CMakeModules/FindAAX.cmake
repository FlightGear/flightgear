# Try to find AAX (AeonWave)
# This module defines
#
# AAX_FOUND - if false, do not try to link to AAX
# AAX_INCLUDE_DIR - where to find the headers
# AAX_LIBRARY - Link these to use AAX
#
# Copyright (C) 2016-2018 by Erik Hofman.
# Copyright (C) 2016-2018 by Adalin B.V.
#
# $AAXDIR is an environment variable that would
# correspond to the ./configure --prefix=$AAXDIR
# used in building AAX.
#
# This file is Public Domain (www.unlicense.org)
# This is free and unencumbered software released into the public domain.

if (AAX_LIBRARY AND AAX_INCLUDE_DIR)
  # in cache already
  set(AAX_FOUND TRUE)
  message(STATUS "Found AeonWave: ${AAX_LIBRARY}")
else()
  find_path(AAX_INCLUDE_DIR aax/aax.h
    HINTS
    $ENV{AAXDIR}
    $ENV{ProgramFiles}/aax
    $ENV{ProgramFiles}/AeonWave
    $ENV{ProgramFiles}/Adalin/AeonWave
    ${CMAKE_SOURCE_DIR}/aax
    PATH_SUFFIXES include
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt
  )

  find_library(AAX_LIBRARY 
    NAMES AAX aax libAAX
    HINTS
    $ENV{AAXDIR}
    $ENV{ProgramFiles}/AAX
    $ENV{ProgramFiles}/AeonWave
    $ENV{ProgramFiles}/Adalin/AeonWave
    ${CMAKE_BUILD_DIR}/aax
    PATH_SUFFIXES lib64 lib lib/${CMAKE_LIBRARY_ARCHITECTURE} libs64 libs libs/Win32 libs/Win64 bin
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt
  )

  set(AAX_DEFINITIONS "")
  if (AAX_LIBRARY AND AAX_INCLUDE_DIR)
    set(AAX_FOUND TRUE)
  endif()

  if (AAX_FOUND)
    if (NOT Udns_FIND_QUIETLY)
      message(STATUS "Found AeonWave: ${AAX_LIBRARY}")
    endif ()
  else ()
    if (Udns_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find AeonWave")
    endif ()
  endif ()

  mark_as_advanced(AAX_LIBRARY AAX_INCLUDE_DIR)

endif()

