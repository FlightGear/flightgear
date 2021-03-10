
set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSuite.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_AIFlightPlan.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_AIManager.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_traffic.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_TrafficMgr.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_groundnet.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_submodels.cxx
    PARENT_SCOPE
)

set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_AIFlightPlan.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_AIManager.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_traffic.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_TrafficMgr.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_groundnet.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/test_submodels.hxx
    PARENT_SCOPE
)
