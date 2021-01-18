set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSuite.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_autosaveMigration.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_posinit.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_timeManager.cxx
    PARENT_SCOPE
)

set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_autosaveMigration.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_posinit.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_timeManager.hxx
    PARENT_SCOPE
)
