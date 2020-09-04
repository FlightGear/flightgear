# - Try to find GSM
# Once done this will define
#
#  GSM_FOUND - system has GSM
#  GSM_INCLUDE_DIRS - the GSM include directory
#  GSM_LIBRARIES - Link these to use GSM
#  GSM_DEFINITIONS - Compiler switches required for using GSM
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#  Edited by Saikrishna Arcot <saiarcot895@gmail.com> to find the GSM library
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (GSM_LIBRARIES AND GSM_INCLUDE_DIRS)
  # in cache already
  set(GSM_FOUND TRUE)
else (GSM_LIBRARIES AND GSM_INCLUDE_DIRS)

  set(GSM_DEFINITIONS "")

  # gsm.h might be at $prefix/include/gsm/gsm.h, but our internal
  # version is at inc/gsm.h, so we need to include that as a PATH_SUFFIX
  # otherwise we get into a mess.
  # see: https://sourceforge.net/p/flightgear/codetickets/2368/
  find_path(GSM_INCLUDE_DIR
    NAMES
      gsm.h
    PATHS
      ${_GsmIncDir}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      gsm
  )

  find_library(GSM_LIBRARY
    NAMES
      gsm
      Gsm
    PATHS
      ${_GsmLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (GSM_LIBRARY)
    set(GSM_FOUND TRUE)
  endif (GSM_LIBRARY)

  set(GSM_INCLUDE_DIRS
    ${GSM_INCLUDE_DIR}
  )

  if (GSM_FOUND)
    set(GSM_LIBRARIES
      ${GSM_LIBRARIES}
      ${GSM_LIBRARY}
    )
  endif (GSM_FOUND)

  if (GSM_INCLUDE_DIRS AND GSM_LIBRARIES)
    set(GSM_FOUND TRUE)
  endif (GSM_INCLUDE_DIRS AND GSM_LIBRARIES)

  if (GSM_FOUND)
    if (NOT Gsm_FIND_QUIETLY)
      message(STATUS "Found GSM: ${GSM_LIBRARIES}")
    endif (NOT Gsm_FIND_QUIETLY)
  else (GSM_FOUND)
    if (Gsm_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find GSM")
    endif (Gsm_FIND_REQUIRED)
  endif (GSM_FOUND)

  # show the GSM_INCLUDE_DIRS and GSM_LIBRARIES variables only in the advanced view
  mark_as_advanced(GSM_INCLUDE_DIRS GSM_LIBRARIES)

endif (GSM_LIBRARIES AND GSM_INCLUDE_DIRS)
