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
	FIND_LIBRARY(APR_LIBS
	  NAMES libapr-1 apr-1
	  HINTS
	  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	  PATHS
	  ~/Library/Frameworks
	  /Library/Frameworks
	  /usr/local
	  /usr
	  /opt
	)
endif(HAVE_APR_CONFIG)

find_path(LIBSVN_INCLUDE_DIR svn_client.h
  HINTS
  $ENV{LIBSVN_DIR}
  PATH_SUFFIXES include/subversion-1
  PATHS
  /usr/local
  /usr
  /opt
)

FIND_LIBRARY(SVNCLIENT_LIBRARY
  NAMES libsvn_client-1 svn_client-1
  HINTS
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

FIND_LIBRARY(SVNSUBR_LIBRARY
  NAMES libsvn_subr-1 svn_subr-1
  HINTS
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

FIND_LIBRARY(SVNRA_LIBRARY
  NAMES libsvn_ra-1 svn_ra-1
  HINTS
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSVN DEFAULT_MSG 
    SVNSUBR_LIBRARY 
    SVNCLIENT_LIBRARY
    SVNRA_LIBRARY 
    LIBSVN_INCLUDE_DIR)

if(LIBSVN_FOUND)
	set(LIBSVN_LIBRARIES ${SVNCLIENT_LIBRARY} ${SVNSUBR_LIBRARY} ${SVNRA_LIBRARY} ${APR_LIBS})
endif(LIBSVN_FOUND)
