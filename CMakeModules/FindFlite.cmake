# - Try to find Flite
# Once done this will define
#
#  FLITE_FOUND - system has Flite
#  FLITE_INCLUDE_DIRS - the Flite include directory
#  FLITE_LIBRARIES - Link these to use Flite
#  FLITE_DEFINITIONS - Compiler switches required for using Flite
#
#  Copyright (c) 2013 Saikrishna Arcot <saiarcot895@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (FLITE_LIBRARIES AND FLITE_INCLUDE_DIRS)
  # in cache already
  set(FLITE_FOUND TRUE)
else (FLITE_LIBRARIES AND FLITE_INCLUDE_DIRS)
  #set(FLITE_DEFINITIONS ${_FliteCflags})
  set(FLITE_DEFINITIONS "")

  find_path(FLITE_INCLUDE_DIR
    NAMES
      flite.h
    PATHS
      ${_FliteIncDir}
      /usr/include
      /usr/include/flite
      /usr/local/include
      /usr/local/include/flite
      /opt/local/include
      /opt/local/include/flite
      /sw/include
      /sw/include/flite
  )

  find_library(FLITE_LIBRARY
    NAMES
      flite
      Flite
    PATHS
      ${_FliteLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  find_library(FLITE_CMU_US_KAL_LIBRARY
    NAMES
      flite_cmu_us_kal
    PATHS
      ${_FliteLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  find_library(FLITE_CMULEX_LIBRARY
    NAMES
      flite_cmulex
    PATHS
      ${_FliteLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  find_library(FLITE_USENGLISH_LIBRARY
    NAMES
      flite_usenglish
    PATHS
      ${_FliteLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (FLITE_LIBRARY AND FLITE_CMU_US_KAL_LIBRARY AND FLITE_CMULEX_LIBRARY AND FLITE_USENGLISH_LIBRARY)
    set(FLITE_FOUND TRUE)
  endif (FLITE_LIBRARY AND FLITE_CMU_US_KAL_LIBRARY AND FLITE_CMULEX_LIBRARY AND FLITE_USENGLISH_LIBRARY)

  set(FLITE_INCLUDE_DIRS
    ${FLITE_INCLUDE_DIR}
  )

  if (FLITE_FOUND)
    set(FLITE_LIBRARIES
      ${FLITE_LIBRARIES}
      ${FLITE_LIBRARY}
      ${FLITE_CMU_US_KAL_LIBRARY}
      ${FLITE_CMULEX_LIBRARY}
      ${FLITE_USENGLISH_LIBRARY}
    )
  endif (FLITE_FOUND)

  if (FLITE_INCLUDE_DIRS AND FLITE_LIBRARIES)
    set(FLITE_FOUND TRUE)
  endif (FLITE_INCLUDE_DIRS AND FLITE_LIBRARIES)

  if (FLITE_FOUND)
    if (NOT Flite_FIND_QUIETLY)
      message(STATUS "Found Flite and needed components: ${FLITE_LIBRARIES}")
    endif (NOT Flite_FIND_QUIETLY)


    if(NOT TARGET Flightgear::Flite)
          add_library(Flightgear::Flite UNKNOWN IMPORTED)
          set_target_properties(Flightgear::Flite PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FLITE_INCLUDE_DIRS}"
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
              IMPORTED_LOCATION "${FLITE_LIBRARY}"
              INTERFACE_LINK_LIBRARIES "${FLITE_USENGLISH_LIBRARY};${FLITE_CMU_US_KAL_LIBRARY};${FLITE_CMULEX_LIBRARY}")
    endif()

  else (FLITE_FOUND)
    if (Flite_FIND_REQUIRED)
	    message(FATAL_ERROR "Could not find Flite or all components of Flite")
    endif (Flite_FIND_REQUIRED)
  endif (FLITE_FOUND)

  # show the FLITE_INCLUDE_DIRS and FLITE_LIBRARIES variables only in the advanced view
  mark_as_advanced(FLITE_INCLUDE_DIRS FLITE_LIBRARIES)

endif (FLITE_LIBRARIES AND FLITE_INCLUDE_DIRS)
