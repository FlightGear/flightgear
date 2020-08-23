# - Try to find HTS Engine
# Once done this will define
#
#  HTS_ENGINE_FOUND - system has HTS Engine
#  HTS_ENGINE_INCLUDE_DIRS - the HTS Engine include directory
#  HTS_ENGINE_LIBRARIES - Link these to use HTS Engine
#  HTS_ENGINE_DEFINITIONS - Compiler switches required for using HTS Engine
#
#  Copyright (c) 2013 Saikrishna Arcot <saiarcot895@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (HTS_ENGINE_LIBRARIES AND HTS_ENGINE_INCLUDE_DIRS)
  # in cache already
  set(HTS_ENGINE_FOUND TRUE)
else (HTS_ENGINE_LIBRARIES AND HTS_ENGINE_INCLUDE_DIRS)
  #set(HTS_ENGINE_DEFINITIONS ${_HTS_EngineCflags})
  set(HTS_ENGINE_DEFINITIONS "")

  find_path(HTS_ENGINE_INCLUDE_DIR
    NAMES
    HTS_engine.h
    PATHS
      ${_HTS_EngineIncDir}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(HTS_ENGINE_LIBRARY
    NAMES
      HTSEngine
    PATHS
      ${_HTS_EngineLinkDir}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (HTS_ENGINE_LIBRARY)
    set(HTS_ENGINE_FOUND TRUE)
  endif (HTS_ENGINE_LIBRARY)

  set(HTS_ENGINE_INCLUDE_DIRS
    ${HTS_ENGINE_INCLUDE_DIR}
  )

  if (HTS_ENGINE_FOUND)
    set(HTS_ENGINE_LIBRARIES
      ${HTS_ENGINE_LIBRARIES}
      ${HTS_ENGINE_LIBRARY}
    )
  endif (HTS_ENGINE_FOUND)

  if (HTS_ENGINE_INCLUDE_DIRS AND HTS_ENGINE_LIBRARIES)
    set(HTS_ENGINE_FOUND TRUE)
  endif (HTS_ENGINE_INCLUDE_DIRS AND HTS_ENGINE_LIBRARIES)

  if (HTS_ENGINE_FOUND)
    if (NOT HTS_Engine_FIND_QUIETLY)
      message(STATUS "Found HTS Engine: ${HTS_ENGINE_LIBRARIES}")
    endif (NOT HTS_Engine_FIND_QUIETLY)

    if(NOT TARGET Flightgear::HTSEngine)
      add_library(Flightgear::HTSEngine UNKNOWN IMPORTED)
      set_target_properties(Flightgear::HTSEngine PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HTS_ENGINE_INCLUDE_DIRS}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          IMPORTED_LOCATION "${HTS_ENGINE_LIBRARY}")
    endif()

  else (HTS_ENGINE_FOUND)
    if (HTS_Engine_FIND_REQUIRED)
	    message(FATAL_ERROR "Could not find HTS Engine")
    endif (HTS_Engine_FIND_REQUIRED)
  endif (HTS_ENGINE_FOUND)

  # show the HTS_ENGINE_INCLUDE_DIRS and HTS_ENGINE_LIBRARIES variables only in the advanced view
  mark_as_advanced(HTS_ENGINE_INCLUDE_DIRS HTS_ENGINE_LIBRARIES)

endif (HTS_ENGINE_LIBRARIES AND HTS_ENGINE_INCLUDE_DIRS)

