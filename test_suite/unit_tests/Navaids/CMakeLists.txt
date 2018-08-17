set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSuite.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_flightplan.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_navaids2.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_aircraftPerformance.cxx
    PARENT_SCOPE
)

set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_flightplan.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_aircraftPerformance.hxx
    PARENT_SCOPE
)
