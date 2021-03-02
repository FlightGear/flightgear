set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSuite.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/testDigitalFilter.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/testPidController.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/testPidControllerData.cxx
    PARENT_SCOPE
)

set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/testDigitalFilter.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/testPidController.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/testPidControllerData.hxx
    PARENT_SCOPE
)
