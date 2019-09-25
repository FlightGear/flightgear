set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSuite.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_navRadio.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gps.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_hold_controller.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rnav_procedures.cxx
    PARENT_SCOPE
)

set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_navRadio.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gps.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_hold_controller.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rnav_procedures.hxx
    PARENT_SCOPE
)
