set(CPPUNIT_SOURCES
    ${CPPUNIT_SOURCES}
)
set(CPPUNIT_HEADERS
    ${CPPUNIT_HEADERS}
)

# Add the src and header directories.
add_subdirectory(src)
add_subdirectory(include)

foreach(s ${CPPUNIT_SOURCES})
    set_property(GLOBAL APPEND PROPERTY CPPUNIT_SOURCES "${s}")
endforeach()

foreach(h ${CPPUNIT_HEADERS})
    set_property(GLOBAL APPEND PROPERTY CPPUNIT_HEADERS "${h}")
endforeach()
