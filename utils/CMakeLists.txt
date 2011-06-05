add_subdirectory(TerraSync)
add_subdirectory(fgviewer)
add_subdirectory(GPSsmooth)

if (FLTK_FOUND)
    add_subdirectory(fgadmin)
endif (FLTK_FOUND)

if (WITH_FGPANEL)
    add_subdirectory(fgpanel)
endif (WITH_FGPANEL)
