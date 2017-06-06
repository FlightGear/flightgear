include(GNUInstallDirs)
set(MANPAGES
    fgfs.1
)

if(ENABLE_FGCOM)
    list(APPEND MANPAGES fgcom.1)
endif(ENABLE_FGCOM)

if(ENABLE_FGELEV)
    list(APPEND MANPAGES fgelev.1)
endif(ENABLE_FGELEV)

if(ENABLE_FGJS)
    list(APPEND MANPAGES fgjs.1)
endif(ENABLE_FGJS)

if(WITH_FGPANEL)
    list(APPEND MANPAGES fgpanel.1)
endif()

if(ENABLE_FGQCANVAS)
    list(APPEND MANPAGES fgqcanvas.1)
endif(ENABLE_FGQCANVAS)

if(ENABLE_FGVIEWER)
    list(APPEND MANPAGES fgviewer.1)
endif(ENABLE_FGVIEWER)

if(ENABLE_GPSSMOOTH)
    list(APPEND MANPAGES GPSsmooth.1 MIDGsmooth.1 UGsmooth.1)
endif(ENABLE_GPSSMOOTH)

if(ENABLE_JS_DEMO)
    list(APPEND MANPAGES js_demo.1)
endif(ENABLE_JS_DEMO)

if(ENABLE_JSBSIM)
    list(APPEND MANPAGES JSBSim.1)
endif(ENABLE_JSBSIM)

if(ENABLE_METAR)
    list(APPEND MANPAGES metar.1)
endif(ENABLE_METAR)

foreach(man ${MANPAGES})
    install(FILES ${man} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
            COMPONENT doc)
endforeach()
