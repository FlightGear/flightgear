# Find Sqlite3
# ~~~~~~~~~~~~
# Copyright (c) 2007, Martin Dobias <wonder.sk at gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# CMake module to search for Sqlite3 library
#
# If it's found it sets SQLITE3_FOUND to TRUE
# and following variables are set:
#    SQLITE3_INCLUDE_DIR
#    SQLITE3_LIBRARY


# FIND_PATH and FIND_LIBRARY normally search standard locations
# before the specified paths. To search non-standard paths first,
# FIND_* is invoked first with specified paths and NO_DEFAULT_PATH
# and then again with no specified paths to search the default
# locations. When an earlier FIND_* succeeds, subsequent FIND_*s
# searching for the same item do nothing. 

FIND_PATH(SQLITE3_INCLUDE_DIR sqlite3.h
    PATH_SUFFIXES include 
    HINTS $ENV{SQLITE3DIR}
    PATHS
    ${ADDITIONAL_LIBRARY_PATHS}
  )

FIND_LIBRARY(SQLITE3_LIBRARY NAMES sqlite3 sqlite3
    HINTS $ENV{SQLITE3DIR}
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS ${ADDITIONAL_LIBRARY_PATHS}
  )

find_package(Threads)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLITE3 DEFAULT_MSG SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)

if(NOT TARGET fgsqlite3)
      add_library(fgsqlite3 UNKNOWN IMPORTED)
      set_target_properties(fgsqlite3 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
          INTERFACE_LINK_LIBRARIES Threads::Threads)
endif()
