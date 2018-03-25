# CMake module includes.
include(SetupFGFSEmbeddedResources)
include(SetupFGFSLibraries)
include(SetupMSVCGrouping)

# Add each test suite category.
foreach(test_category
        gui_tests
        simgear_tests
        system_tests
        unit_tests
    )
    add_subdirectory(${test_category})
endforeach(test_category)

# Add the helpers.
add_subdirectory(helpers)

# Add all test suite sources and headers.
set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    bootstrap.cxx
    fgCompilerOutputter.cxx
    fgTestListener.cxx
    fgTestRunner.cxx
    formatting.cxx
    logging.cxx
    testSuite.cxx
)
set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    fgCompilerOutputter.hxx
    fgTestListener.hxx
    fgTestRunner.hxx
    formatting.hxx
    logging.hxx
)

# The test suite output directory.
set(TESTSUITE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Get the CppUnit sources and headers to be built into the test suite.
if (NOT SYSTEM_CPPUNIT)
    message(STATUS "CppUnit: Building the FlightGear supplied CppUnit library")

    get_property(CPPUNIT_SOURCES GLOBAL PROPERTY CPPUNIT_SOURCES)
    get_property(CPPUNIT_HEADERS GLOBAL PROPERTY CPPUNIT_HEADERS)

    include_directories("${PROJECT_SOURCE_DIR}/3rdparty/cppunit/include")

    add_library(CppUnitLib STATIC ${CPPUNIT_SOURCES} ${CPPUNIT_HEADERS})
    set(CPPUNIT_LIBRARIES CppUnitLib)
else()
    message(STATUS "CppUnit: Linking to the system supplied CppUnit library")
endif()

#-----------------------------------------------------------------------------
# Set up all test suites as CTests.

# System test suites.

# Unit test suites.
add_test(AddonManagementUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u AddonManagementTests)
add_test(FlightplanUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u FlightplanTests)
add_test(LaRCSimMatrixUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u LaRCSimMatrixTests)
add_test(MktimeUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u MktimeTests)
add_test(NasalSysUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u NasalSysTests)
add_test(PosInitUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u PosInitTests)

# GUI test suites.

# Simgear unit test suites.
add_test(MathGeodesySimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -m MathGeodesyTests)
add_test(SimgearPropsSimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -m SimgearPropsTests)

#-----------------------------------------------------------------------------
# Set up the binary.

# Set up the Main FG file sources and headers (excluding bootstrap.cxx and its main() function).
if(MSVC)
    set(RESOURCE_FILE ${PROJECT_SOURCE_DIR}/src/Main/flightgear.rc)
endif(MSVC)

set(MAIN_SOURCES
    #${PROJECT_SOURCE_DIR}/src/Main/bootstrap.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_commands.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_init.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_io.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_os_common.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_scene_commands.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_props.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/FGInterpolator.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/globals.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/locale.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/logger.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/main.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/options.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/positioninit.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/screensaver_control.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/subsystemFactory.cxx
    ${PROJECT_SOURCE_DIR}/src/Main/util.cxx
    ${RESOURCE_FILE}
)

set(MAIN_HEADERS
    ${PROJECT_SOURCE_DIR}/src/Main/AircraftDirVisitorBase.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_commands.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_init.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_io.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/fg_props.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/FGInterpolator.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/globals.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/locale.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/logger.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/main.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/options.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/positioninit.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/screensaver_control.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/subsystemFactory.hxx
    ${PROJECT_SOURCE_DIR}/src/Main/util.hxx
)

# Set up the embedded resources.
setup_fgfs_embedded_resources()

# Sort the sources and headers for MSVC.
setup_msvc_grouping()

# All sources and headers to be built into the test suite.
get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)
get_property(EMBEDDED_RESOURCE_SOURCES GLOBAL PROPERTY EMBEDDED_RESOURCE_SOURCES)
get_property(EMBEDDED_RESOURCE_HEADERS GLOBAL PROPERTY EMBEDDED_RESOURCE_HEADERS)

# Set up the separate executable for running the test suite.
add_executable(run_test_suite
    ${FG_SOURCES}
    ${FG_HEADERS}
    ${EMBEDDED_RESOURCE_SOURCES}
    ${EMBEDDED_RESOURCE_HEADERS}
    ${MAIN_SOURCES}
    ${MAIN_HEADERS}
    ${TESTSUITE_SOURCES}
    ${TESTSUITE_HEADERS}
)
set_target_properties(run_test_suite
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TESTSUITE_OUTPUT_DIR}"
)
if(ENABLE_AUTOTESTING)
    set(TEST_SUITE_COMMENT "Running the full FlightGear test suite.")
    if(WIN32)
        set(TEST_SUITE_COMMAND "run_test_suite.exe")
        set(TEST_SUITE_WORKING_DIRECTORY "${TESTSUITE_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}")
    else()
        set(TEST_SUITE_COMMAND "run_test_suite")
        set(TEST_SUITE_WORKING_DIRECTORY "${TESTSUITE_OUTPUT_DIR}")
    endif(WIN32)
else()
    set(TEST_SUITE_COMMENT "Building the FlightGear test suite.")
endif(ENABLE_AUTOTESTING)

add_custom_target(test_suite
    ${TEST_SUITE_COMMAND}
    DEPENDS run_test_suite
    WORKING_DIRECTORY ${TEST_SUITE_WORKING_DIRECTORY}
    COMMENT ${TEST_SUITE_COMMENT}
)

# Set up the target links and includes.
setup_fgfs_libraries(run_test_suite)

# Additional libraries just for the test suite.
target_link_libraries(run_test_suite ${CPPUNIT_LIBRARIES})
