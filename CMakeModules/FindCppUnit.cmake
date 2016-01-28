# Locate CppUnit.
#
# This module defines
# CPPUNIT_FOUND
# CPPUNIT_LIBRARIES
# CPPUNIT_INCLUDE_DIR

# Find CppUnit.
if (NOT CPPUNIT_LIBRARIES AND NOT CPPUNIT_INCLUDE_DIR)
    # Find the headers.
    find_path(CPPUNIT_INCLUDE_DIR
        NAMES
            cppunit/Test.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/include
            /opt/local/include
            /sw/include
    )

    # Find the library.
    find_library(CPPUNIT_LIBRARIES
        NAMES
            cppunit
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/lib
            /opt/local/lib
            /sw/lib
    )

endif ()


# Pre-set or found.
if (CPPUNIT_LIBRARIES AND CPPUNIT_INCLUDE_DIR)
    set(CPPUNIT_FOUND TRUE)
    message(STATUS "CppUnit library found: ${CPPUNIT_LIBRARIES}")
    message(STATUS "CppUnit include directory found: ${CPPUNIT_INCLUDE_DIR}")
endif ()
