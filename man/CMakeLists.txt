if (MSVC)
    # don't install man pages on Windows
else(MSVC)

if(${CMAKE_VERSION} VERSION_GREATER 2.8.4)
    # use official include provided by latest CMake
    include(GNUInstallDirs)
else(${CMAKE_VERSION} VERSION_GREATER 2.8.4)
    # backward compatibility: use our own module for older cmake versions
    include(OldGNUInstallDirs)
endif(${CMAKE_VERSION} VERSION_GREATER 2.8.4)

set(MANPAGES
    fgfs.1
    fgjs.1
    fgpanel.1
    gl-info.1
    js_demo.1
    terrasync.1)

# map some variables to what the man-pages expect
set(PACKAGE ${CMAKE_PROJECT_NAME})
set(VERSION ${FLIGHTGEAR_VERSION})

foreach(man ${MANPAGES})
    configure_file (
        "${PROJECT_SOURCE_DIR}/man/${man}.in"
        "${PROJECT_BINARY_DIR}/man/${man}"
        )

    install(FILES ${PROJECT_BINARY_DIR}/man/${man}
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 COMPONENT doc)
endforeach()

endif(MSVC)