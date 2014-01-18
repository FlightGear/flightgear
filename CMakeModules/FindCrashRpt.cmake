# Find CrashRpt
# ~~~~~~~~~~~~
# Copyright (c) 2014, James Turner <zakalawe at mac dot com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# CMake module to search for CrashRpt library
#
# If it's found it sets CRASHRPT_FOUND to TRUE
# and following variables are set:
#    CRASHRPT_FOUND_INCLUDE_DIR
#    CRASHRPT_FOUND_LIBRARY

FIND_PATH(CRASHRPT_INCLUDE_DIR CrashRpt.h
    PATH_SUFFIXES include 
    HINTS $ENV{CRASHRPTDIR}
    PATHS
    ${ADDITIONAL_LIBRARY_PATHS}
  )

set(CRASHRPPT_LIBRARIES "")
  
FIND_LIBRARY(CRASHRPT_LIBRARY NAMES CrashRpt1402
    HINTS $ENV{CRASHRPTDIR}
    PATH_SUFFIXES lib
    PATHS ${ADDITIONAL_LIBRARY_PATHS}
  )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CRASHRPT DEFAULT_MSG 
	CRASHRPT_LIBRARY CRASHRPT_INCLUDE_DIR)
