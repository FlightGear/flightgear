include(GnuInstallDirs)

set(MANPAGES
    est-epsilon.1
    fgfs.1
    fgjs.1
    gl-info.1
    js_demo.1
    pstest.1)

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
  
