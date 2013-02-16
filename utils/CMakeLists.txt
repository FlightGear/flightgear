add_subdirectory(TerraSync)
add_subdirectory(fgviewer)
add_subdirectory(fgelev)
add_subdirectory(GPSsmooth)

if (FLTK_FOUND)
    if (EXISTS ${FLTK_FLUID_EXECUTABLE})
        add_subdirectory(fgadmin)
    else ()
        message(STATUS "fluid executable not found, disabling fgadmin")
    endif ()
endif (FLTK_FOUND)

if (WITH_FGPANEL)
    add_subdirectory(fgpanel)
endif (WITH_FGPANEL)

# win32 is just excluded because of not having argument parsing there ...
if(RTI_FOUND AND NOT WIN32)
    add_subdirectory(fgai)
endif(RTI_FOUND AND NOT WIN32)
