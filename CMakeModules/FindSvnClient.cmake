# Find Subversion client libraries, and dependencies
# including APR (Apache Portable Runtime)

include (CheckFunctionExists)
include (CheckIncludeFile)

find_program(HAVE_APR_CONFIG apr-1-config)
if(HAVE_APR_CONFIG) 
        
    execute_process(COMMAND apr-1-config --cppflags --includes
        OUTPUT_VARIABLE APR_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        
    execute_process(COMMAND apr-1-config --link-ld
        OUTPUT_VARIABLE RAW_APR_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    
# clean up some vars, or other CMake pieces complain
	string(STRIP ${RAW_APR_LIBS} APR_LIBS)

else(HAVE_APR_CONFIG)
    message(STATUS "apr-1-config not found, implement manual search for APR")
endif(HAVE_APR_CONFIG)

if(HAVE_APR_CONFIG)
	find_path(LIBSVN_INCLUDE_DIR svn_client.h
	  HINTS
	  $ENV{LIBSVN_DIR}
	  PATH_SUFFIXES include/subversion-1
	  PATHS
	  /usr/local
	  /usr
	  /opt
	)

	check_library_exists(svn_client-1 svn_client_checkout "" HAVE_LIB_SVNCLIENT)
	check_library_exists(svn_subr-1 svn_cmdline_init "" HAVE_LIB_SVNSUBR)
	check_library_exists(svn_ra-1 svn_ra_initialize "" HAVE_LIB_SVNRA)

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSVN DEFAULT_MSG 
		HAVE_LIB_SVNSUBR 
		HAVE_LIB_SVNCLIENT
		HAVE_LIB_SVNRA 
		LIBSVN_INCLUDE_DIR)

	if(LIBSVN_FOUND)
		set(LIBSVN_LIBRARIES "svn_client-1" "svn_subr-1" "svn_ra-1" ${APR_LIBS})
	endif(LIBSVN_FOUND)
endif(HAVE_APR_CONFIG)
