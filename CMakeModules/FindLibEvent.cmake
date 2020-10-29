# Finding LibEvent (https://libevent.org/)
# Defining:
# LIBEVENT_LIB
# LIBEVENT_INCLUDE_DIR

set(libEvent_target "_no_target_")

if(WIN32)
	FIND_PATH(LIBEVENT_INCLUDE_DIR event2/event.h PATH_SUFFIXES include HINTS ${ADDITIONAL_LIBRARY_PATHS})
	FIND_LIBRARY(LIBEVENT_LIB NAMES event_core PATH_SUFFIXES lib HINTS ${ADDITIONAL_LIBRARY_PATHS})
	if (LIBEVENT_INCLUDE_DIR AND LIBEVENT_LIB)
		add_library(libEvent UNKNOWN IMPORTED)
		set_target_properties(libEvent PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${LIBEVENT_INCLUDE_DIR}"
			IMPORTED_LOCATION "${LIBEVENT_LIB}"
		)
		set(libEvent_target "libEvent")
	endif()


else()
	find_package(PkgConfig QUIET)

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(libEvent  IMPORTED_TARGET libevent)
	endif()

	if(libEvent_FOUND)
		# can't use an ALIAS here, needs CMake 3.11
		# use a global property instead
		set(libEvent_target "PkgConfig::libEvent")
	endif()
endif(WIN32)
