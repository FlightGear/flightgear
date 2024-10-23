if (ENABLE_SWIFT)
    # DBUS
    # our local FindDBus.cmake file uses pkg-config on non-Windows
    # we want to ensure our local prefixes are searched, so set this
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH 1)

    # unfortunately CMAKE_INSTALL_PREFIX is not searched, so add that manually
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

    find_package(DBus)
    find_package(LibEvent)

    if (TARGET DBus::DBus AND TARGET libEvent::libEvent)
        message(STATUS "SWIFT support enabled")
    else()
        message(STATUS "SWIFT support disabled, dbus and/or LibEvent not found")
        set(ENABLE_SWIFT 0)
    endif()
endif()
