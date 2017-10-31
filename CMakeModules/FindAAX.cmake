# Locate AAX
# This module defines
# AAX_LIBRARIES
# AAX_FOUND, if false, do not try to link to AAX 
# AAX_INCLUDE_DIR, where to find the headers
#
# $AAXDIR is an environment variable that would
# correspond to the ./configure --prefix=$AAXDIR
# used in building AAX.
#
# Created by Erik Hofman.

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
  NAMES AAX aax AAX32
  HINTS
  $ENV{AAXDIR}
  $ENV{ProgramFiles}/AAX
  $ENV{ProgramFiles}/AeonWave
  $ENV{ProgramFiles}/Adalin/AeonWave
  ${CMAKE_BUILD_DIR}/aax
  PATH_SUFFIXES bin lib lib/${CMAKE_LIBRARY_ARCHITECTURE} lib64 libs64 libs libs/Win32 libs/Win64
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
      SET(AAX_LIBRARY "${aax_BUILD_DIR}/aax/AAX32.dll")
      SET(AAX_FOUND "YES")
    ELSE(SINGLE_PACKAGE)
    ENDIF(SINGLE_PACKAGE)
  ENDIF(NOT AAX_LIBRARY)
ENDIF(AAX_LIBRARY AND AAX_INCLUDE_DIR)

