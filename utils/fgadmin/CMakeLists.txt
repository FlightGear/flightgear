#Zlib is already required by FG but we check it just in case
if( ZLIB_FOUND )
    include_directories(${ZLIB_INCLUDE_DIR})
    set( HAVE_ZLIB 1)
    message(STATUS "Zlib found for fgadmin")
endif( ZLIB_FOUND )

find_package(BZip2)
if( BZIP2_FOUND )
    include_directories(${BZIP2_INCLUDE_DIR})
    set( HAVE_BZ2LIB 1)
    message(STATUS "BZip2 found for fgadmin")
endif( BZIP2_FOUND )

find_package(FLTK)
if( FLTK_FOUND )
    include_directories(${FLTK_INCLUDE_DIR})

    if ( X11_Xinerama_FOUND )
        message(STATUS "Found X11_Xinerama...")
        list(APPEND FLTK_LIBRARIES ${X11_Xinerama_LIB})
    endif()

    if ( X11_Xft_FOUND )
        message(STATUS "Found X11_Xft...")
        list(APPEND FLTK_LIBRARIES ${X11_Xft_LIB})
    endif()

    if ( CMAKE_DL_LIBS )
        list(APPEND FLTK_LIBRARIES ${CMAKE_DL_LIBS})
    endif()

    if( EXISTS ${FLTK_FLUID_EXECUTABLE} )
        add_subdirectory(src)
    else()
        message(STATUS "Fluid executable not found, disabling fgadmin")
    endif()

    message(STATUS "Using FLTK_LIBRARIES for fgadmin: ${FLTK_LIBRARIES}")
else()
    message(STATUS "FLTK not found, disabling fgadmin")
endif( FLTK_FOUND )

configure_file(
    "${PROJECT_SOURCE_DIR}/utils/fgadmin/src/config_cmake.h.in"
    "${PROJECT_BINARY_DIR}/utils/fgadmin/src/config_fgadmin.h"
)
