# Find Subversion client libraries, and dependencies
# including APR (Apache Portable Runtime)

include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckLibraryExists)

macro(find_static_component comp libs)
    # account for alternative Windows svn distribution naming
    if(MSVC)
      set(compLib "lib${comp}")
    else(MSVC)
      set(compLib "${comp}")
    endif(MSVC)
    
    string(TOUPPER "${comp}" compLibBase)
    set( compLibName ${compLibBase}_LIBRARY )

    # NO_DEFAULT_PATH is important on Mac - we need to ensure subversion
    # libraires in dist/ or Macports are picked over the Apple version 
    # in /usr, since that's what we will ship.
    # On other platforms we do need default paths though, i.e. since Linux
    # distros may use architecture-specific directories (like
    # /usr/lib/x86_64-linux-gnu) which we cannot hardcode/guess here.
    FIND_LIBRARY(${compLibName}
if(APPLE)
      NO_DEFAULT_PATH
endif(APPLE)
      NAMES ${compLib}
      HINTS $ENV{LIBSVN_DIR} ${CMAKE_INSTALL_PREFIX} ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS ${ADDITIONAL_LIBRARY_PATHS}
    )

	list(APPEND ${libs} ${${compLibName}})
endmacro()

find_program(HAVE_APR_CONFIG apr-1-config)
if(HAVE_APR_CONFIG) 
        
    execute_process(COMMAND apr-1-config --cppflags --includes
        OUTPUT_VARIABLE APR_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        
    execute_process(COMMAND apr-1-config --link-ld
        OUTPUT_VARIABLE RAW_APR_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    
# clean up some vars, or other CMake pieces complain
	string(STRIP "${RAW_APR_LIBS}" APR_LIBS)

else(HAVE_APR_CONFIG)
    message(STATUS "apr-1-config not found, implement manual search for APR")
endif(HAVE_APR_CONFIG)

if(HAVE_APR_CONFIG OR MSVC)
	find_path(LIBSVN_INCLUDE_DIR svn_client.h
      NO_DEFAULT_PATH
	  HINTS
	  $ENV{LIBSVN_DIR} ${CMAKE_INSTALL_PREFIX} ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include
	  PATH_SUFFIXES include/subversion-1
	  PATHS ${ADDITIONAL_LIBRARY_PATHS}
	)
	
	set(LIBSVN_LIBRARIES "")
	if (MSVC)
		find_static_component("apr-1" LIBSVN_LIBRARIES)
	else (MSVC)
		list(APPEND LIBSVN_LIBRARIES ${APR_LIBS})
	endif (MSVC)
	find_static_component("svn_client-1" LIBSVN_LIBRARIES)
	find_static_component("svn_subr-1" LIBSVN_LIBRARIES)
	find_static_component("svn_ra-1" LIBSVN_LIBRARIES)

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSVN DEFAULT_MSG LIBSVN_LIBRARIES LIBSVN_INCLUDE_DIR)
        if(NOT LIBSVN_FOUND)
		set(LIBSVN_LIBRARIES "")
        endif(NOT LIBSVN_FOUND)
endif(HAVE_APR_CONFIG OR MSVC)
