# win32 is just excluded because of not having argument parsing there ...
if(RTI_FOUND AND NOT WIN32)
    add_subdirectory(fgai)
endif()

if(ENABLE_FGELEV)
    add_subdirectory(fgelev)
endif()

if(WITH_FGPANEL)
    add_subdirectory(fgpanel)
endif()

if(ENABLE_FGVIEWER)
    add_subdirectory(fgviewer)
endif()

if(ENABLE_GPSSMOOTH)
    add_subdirectory(GPSsmooth)
endif()

if(ENABLE_TERRASYNC)
    add_subdirectory(TerraSync)
endif()

if(ENABLE_FGCOM)
    add_subdirectory(fgcom)
endif()

if(ENABLE_STGMERGE)
    add_subdirectory(stgmerge)
endif()

if(ENABLE_TRAFFIC)
    add_subdirectory(traffic)
endif()

if (ENABLE_FGQCANVAS)
  add_subdirectory(fgqcanvas)

  if (TARGET fgqcanvas)
    # do this here so standalone build of FGQCanvas works
    install(TARGETS fgqcanvas RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()
