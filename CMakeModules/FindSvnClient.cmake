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

    FIND_LIBRARY(${compLibName}
      NAMES ${compLib}
      HINTS $ENV{PLIBDIR}
      PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
      PATHS
      /usr/local
      /usr
      /opt
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
	  HINTS
	  $ENV{LIBSVN_DIR}
	  PATH_SUFFIXES include/subversion-1
	  PATHS
	  /usr/local
	  /usr
	  /opt
	)
	
	set(LIBSVN_LIBRARIES "")
	if (MSVC)
		find_static_component("apr-1" LIBSVN_LIBRARIES)
	else (MSVC)
		list(APPEND LIBSVN_LIBRARIES APR_LIBS)
	endif (MSVC)
	find_static_component("svn_client-1" LIBSVN_LIBRARIES)
	find_static_component("svn_subr-1" LIBSVN_LIBRARIES)
	find_static_component("svn_ra-1" LIBSVN_LIBRARIES)

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSVN DEFAULT_MSG LIBSVN_LIBRARIES LIBSVN_INCLUDE_DIR)
endif(HAVE_APR_CONFIG OR MSVC)
