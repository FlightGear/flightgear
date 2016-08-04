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

FIND_PATH(AAX_INCLUDE_DIR aeonwave.hpp
  HINTS
  $ENV{AAXDIR}
  $ENV{ProgramFiles}/aax
  $ENV{ProgramFiles}/AeonWave
  $ENV{ProgramFiles}/Adalin/AeonWave
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

FIND_LIBRARY(AAX_LIBRARY 
  NAMES AAX aax AAX32 libAAX32
  HINTS
  $ENV{AAXDIR}
  $ENV{ProgramFiles}/AAX
  $ENV{ProgramFiles}/AeonWave
  $ENV{ProgramFiles}/Adalin/AeonWave
  PATH_SUFFIXES bin lib lib/${CMAKE_LIBRARY_ARCHITECTURE} lib64 libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr
  /opt
  /usr/local
)

SET(AAX_FOUND "NO")
IF(AAX_LIBRARY AND AAX_INCLUDE_DIR)
  SET(AAX_FOUND "YES")
ENDIF(AAX_LIBRARY AND AAX_INCLUDE_DIR)

