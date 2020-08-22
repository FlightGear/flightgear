# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindOpenAL
----------


Finds Open Audio Library (OpenAL).
This module defines ``OPENAL_LIBRARY OPENAL_FOUND``, if
false, do not try to link to OpenAL ``OPENAL_INCLUDE_DIR``, where to find
the headers.

``$OPENALDIR`` is an environment variable that would correspond to the
``./configure --prefix=$OPENALDIR`` used in building OpenAL.

Created by Eric Wing.  This was influenced by the ``FindSDL.cmake``
module.
#]=======================================================================]

# Modifed for FlightGear by James Turner
# We want to use the 'parented' directories here, unlike the standard CMake version
# of this.

if (OPENAL_LIBRARY AND OPENAL_INCLUDE_DIR)
  # already set, do nothing
else()
  find_path(OPENAL_INCLUDE_DIR OpenAL/al.h AL/al.h
    HINTS
      ENV OPENALDIR
    PATH_SUFFIXES include
    PATHS
    /opt
  )

  find_library(OPENAL_LIBRARY
    NAMES OpenAL al openal OpenAL32
    HINTS
      ENV OPENALDIR
    PATH_SUFFIXES libx32 lib64 lib libs64 libs 
    PATHS
    /opt
  )

  find_package_handle_standard_args(OpenAL  DEFAULT_MSG  OPENAL_LIBRARY OPENAL_INCLUDE_DIR)

  mark_as_advanced(OPENAL_LIBRARY OPENAL_INCLUDE_DIR)
endif()
