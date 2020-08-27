# Finding dbus (https://www.freedesktop.org/wiki/Software/dbus/)
# Defining:
# DBUS_LIBRARY
# DBUS_INCLUDE_DIR

set(dbus_target "_no_target_")

if(WIN32)
	FIND_PATH(DBUS_INCLUDE_DIRS dbus/dbus.h PATH_SUFFIXES include HINTS ${ADDITIONAL_LIBRARY_PATHS})
	FIND_LIBRARY(DBUS_LIBRARIES NAMES dbus-1 PATH_SUFFIXES lib HINTS ${ADDITIONAL_LIBRARY_PATHS})

	# define an imported target for DBus manually
	if (DBUS_INCLUDE_DIRS AND DBUS_LIBRARIES)
		add_library(DBus UNKNOWN IMPORTED)
		set_target_properties(DBus PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${DBUS_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${DBUS_LIBRARIES}"
		)

		set(HAVE_DBUS 1)
		set(dbus_target "DBus")
	endif()
else()
	find_package(PkgConfig QUIET)

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(DBUS IMPORTED_TARGET dbus-1)
	endif (PKG_CONFIG_FOUND)

	if(DBUS_FOUND)
		set(HAVE_DBUS 1)

		# can't use an ALIAS here, needs CMake 3.11
		# use a global property instead
		set(dbus_target "PkgConfig::DBUS")
	endif(DBUS_FOUND)
endif(WIN32)
