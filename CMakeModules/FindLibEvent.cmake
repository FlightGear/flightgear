# Finding LibEvent (https://libevent.org/)
# Defining:
# LIBEVENT_LIB
# LIBEVENT_INCLUDE_DIR

find_path(LIBEVENT_INCLUDE_DIR event.h PATHS ${LibEvent_INCLUDE_PATHS})
find_library(LIBEVENT_LIB NAMES event PATHS ${LibEvent_LIB_PATHS})

if(LIBEVENT_INCLUDE_DIR AND LIBEVENT_LIB)
message(STATUS "LibEvent found.")
endif()