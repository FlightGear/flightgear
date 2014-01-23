# - Try to find Speex extended library
# Once done this will define
#
#  SPEEXDSP_FOUND - system has Speex extended library
#  SPEEXDSP_INCLUDE_DIRS - the Speex extended library include directory
#  SPEEXDSP_LIBRARIES - Link these to use Speex extended library
#  SPEEXDSP_DEFINITIONS - Compiler switches required for using Speex extended library
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#  Edited by Saikrishna Arcot <saiarcot895@gmail.com> to find the Speex extended library
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (SPEEXDSP_LIBRARIES AND SPEEXDSP_INCLUDE_DIRS)
  # in cache already
  set(SPEEXDSP_FOUND TRUE)
else (SPEEXDSP_LIBRARIES AND SPEEXDSP_INCLUDE_DIRS)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  #include(UsePkgConfig)

  #FIXME pkgconfig does not work: return a carriage return that makes compilation failed
  #pkgconfig(speexdsp _SpeexDspIncDir _SpeexDspLinkDir _SpeexDspLinkFlags _SpeexDspCflags)

  #set(SPEEXDSP_DEFINITIONS ${_SpeexDspCflags})
  set(SPEEXDSP_DEFINITIONS "")

  find_path(SPEEXDSP_INCLUDE_DIR
    NAMES
      speex/speex_preprocess.h
      speex_preprocess.h
    PATHS
      ${_SpeexDspIncDir}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(SPEEXDSP_LIBRARY
    NAMES
      speexdsp
      Speexdsp
    PATHS
      ${_SpeexDspLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (SPEEXDSP_LIBRARY)
    set(SPEEXDSP_FOUND TRUE)
  endif (SPEEXDSP_LIBRARY)

  set(SPEEXDSP_INCLUDE_DIRS
    ${SPEEXDSP_INCLUDE_DIR}
  )

  if (SPEEXDSP_FOUND)
    set(SPEEXDSP_LIBRARIES
      ${SPEEXDSP_LIBRARIES}
      ${SPEEXDSP_LIBRARY}
    )
  endif (SPEEXDSP_FOUND)

  if (SPEEXDSP_INCLUDE_DIRS AND SPEEXDSP_LIBRARIES)
    set(SPEEXDSP_FOUND TRUE)
  endif (SPEEXDSP_INCLUDE_DIRS AND SPEEXDSP_LIBRARIES)

  if (SPEEXDSP_FOUND)
    if (NOT Speexdsp_FIND_QUIETLY)
      message(STATUS "Found Speex extended library: ${SPEEXDSP_LIBRARIES}")
    endif (NOT Speexdsp_FIND_QUIETLY)
  else (SPEEXDSP_FOUND)
    if (Speexdsp_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Speex extended library")
    endif (Speexdsp_FIND_REQUIRED)
  endif (SPEEXDSP_FOUND)

  # show the SPEEXDSP_INCLUDE_DIRS and SPEEXDSP_LIBRARIES variables only in the advanced view
  mark_as_advanced(SPEEXDSP_INCLUDE_DIRS SPEEXDSP_LIBRARIES)

endif (SPEEXDSP_LIBRARIES AND SPEEXDSP_INCLUDE_DIRS)
