# Try to find AAX (AeonWave)
# This module defines
#
# AAX_FOUND - if false, do not try to link to AAX
# AAX_INCLUDE_DIR - where to find the headers
# AAX_LIBRARIES - Link these to use AAX
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

FIND_PATH(AAX_INCLUDE_DIR aax/aax.h
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

FIND_LIBRARY(AAX_LIBRARY 
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

IF(AAX_LIBRARY AND AAX_INCLUDE_DIR)
  SET(AAX_FOUND "YES")
ELSE(AAX_LIBRARY AND AAX_INCLUDE_DIR)
  IF(NOT AAX_INCLUDE_DIR)
    MESSAGE(FATAL_ERROR "Unable to find the AAX library development files.")
    SET(AAX_FOUND "NO")
  ENDIF(NOT AAX_INCLUDE_DIR)
  IF(NOT AAX_LIBRARY)
    IF(SINGLE_PACKAGE)
      SET(AAX_LIBRARY "${aax_BUILD_DIR}/aax/AAX.lib")
      SET(AAX_FOUND "YES")
    ELSE(SINGLE_PACKAGE)
    ENDIF(SINGLE_PACKAGE)
  ENDIF(NOT AAX_LIBRARY)
ENDIF(AAX_LIBRARY AND AAX_INCLUDE_DIR)

