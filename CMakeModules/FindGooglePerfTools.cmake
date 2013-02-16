# - Try to find GooglePerfTools headers and libraries
#
# Usage of this module as follows:
#
#   find_package(GooglePerfTools)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  GooglePerfTools_ROOT_DIR  Set this variable to the root installation of
#                            GooglePerfTools if the module has problems finding
#                            the proper installation path.
#
# Variables defined by this module:
#
#  GooglePerfTools_FOUND              System has GooglePerfTools libs/headers
#  GooglePerfTools_LIBRARIES          The GooglePerfTools libraries
#  GooglePerfTools_INCLUDE_DIR        The location of GooglePerfTools headers

find_path(GooglePerfTools_ROOT_DIR
  NAMES include/google/profiler.h
)

find_path(GooglePerfTools_INCLUDE_DIR
  NAMES google/profiler.h
  HINTS ${GooglePerfTools_ROOT_DIR}
)

find_library(GooglePerfTools_PROFILER_LIBRARY
  NAMES profiler
  HINTS ${GooglePerfTools_ROOT_DIR}
)

set(GooglePerfTools_LIBRARIES ${GooglePerfTools_PROFILER_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GooglePerfTools
  "Try setting GooglePerfTools_ROOT_DIR to root of your gperftools installation"
  GooglePerfTools_LIBRARIES
  GooglePerfTools_INCLUDE_DIR
)

mark_as_advanced(
  GooglePerfTools_ROOT_DIR
  GooglePerfTools_LIBRARIES
  GooglePerfTools_PROFILER_LIBRARY
  GooglePerfTools_INCLUDE_DIR
)