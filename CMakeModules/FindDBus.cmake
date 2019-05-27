# Finding dbus (https://www.freedesktop.org/wiki/Software/dbus/)
# Defining:
# DBUS_LIBRARY
# DBUS_INCLUDE_DIR

if(WIN32)
	FIND_PATH(DBUS_INCLUDE_DIRS dbus/dbus.h PATH_SUFFIXES include HINTS ${ADDITIONAL_LIBRARY_PATHS})
	FIND_LIBRARY(DBUS_LIBRARIES NAMES dbus-1 PATH_SUFFIXES lib HINTS ${ADDITIONAL_LIBRARY_PATHS})

	# define an imported target for DBus manually
	if (DBUS_INCLUDE_DIRS AND DBUS_LIBRARIES)
		add_library(DBus UNKNOWN IMPORTED)
		set_target_properties(DBus PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${DBUS_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${DBUS_LIBRARIES)}"
		)
	endif()
else()
	include(FindPkgConfig)

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(DBUS IMPORTED_TARGET dbus-1)
	endif (PKG_CONFIG_FOUND)

	if(DBUS_FOUND)
		set(HAVE_DBUS 1)
	endif(DBUS_FOUND)
endif(WIN32)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DBUS DEFAULT_MSG
	DBUS_INCLUDE_DIRS DBUS_LIBRARIES)