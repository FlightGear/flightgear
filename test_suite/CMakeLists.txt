# CMake module includes.
include(SetupFGFSEmbeddedResources)
include(SetupFGFSIncludes)
include(SetupFGFSLibraries)
include(SetupMSVCGrouping)

# Add each test suite category.
foreach(test_category
        gui_tests
        simgear_tests
        system_tests
        unit_tests
        fgdata_tests
    )
    add_subdirectory(${test_category})
endforeach(test_category)

# Add the testing API.
add_subdirectory(FGTestApi)

# Add all test suite sources and headers.
set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    bootstrap.cxx
    dataStore.cxx
    fgCompilerOutputter.cxx
    fgTestListener.cxx
    fgTestRunner.cxx
    formatting.cxx
    logging.cxx
    testSuite.cxx
)
set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    dataStore.hxx
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

# Activation.
include(Dart)

# System test suites.
add_test(AeroMeshSystemTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -s AeroMeshTests)
#add_test(GPSSystemTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -s GPSTests)
#add_test(NavaidsSystemTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -s NavaidsTests)

# Unit test suites.
add_test(AddonManagementUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u AddonManagementTests)
add_test(AeroElementUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u AeroElementTests)
add_test(AircraftPerformanceTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u AircraftPerformanceTests)
add_test(AutosaveMigrationUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u AutosaveMigrationTests)
add_test(FlightplanUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u FlightplanTests)
if(ENABLE_HID_INPUT)
    add_test(HIDInputUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u HIDInputTests)
endif()
add_test(LaRCSimMatrixUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u LaRCSimMatrixTests)
add_test(MktimeUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u MktimeTests)
add_test(NasalSysUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u NasalSysTests)
add_test(NavaidsUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u NavaidsTests)
add_test(NavRadioTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u NavRadioTests)
add_test(PosInitUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u PosInitTests)
add_test(YASimAtmosphereUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -u YASimAtmosphereTests)

# GUI test suites.

# Simgear unit test suites.
add_test(MathGeodesySimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -m MathGeodesyTests)
add_test(SimgearPropsSimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -m SimgearPropsTests)

# FGData test suites.

#-----------------------------------------------------------------------------
# Set up the binary.

# Set up the embedded resources.
setup_fgfs_embedded_resources()

# Sort the sources and headers for MSVC.
setup_msvc_grouping()

# Additional search paths for includes.
setup_fgfs_includes()

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
    ${TESTSUITE_SOURCES}
    ${TESTSUITE_HEADERS}
)
add_dependencies(run_test_suite buildId)
set_target_properties(run_test_suite
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TESTSUITE_OUTPUT_DIR}"
)
if(ENABLE_AUTOTESTING)
    if(WIN32)
        set(TEST_SUITE_COMMAND "run_test_suite.exe")
        set(TEST_SUITE_WORKING_DIRECTORY "${TESTSUITE_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}")
    else()
        set(TEST_SUITE_COMMAND "run_test_suite")
        set(TEST_SUITE_WORKING_DIRECTORY "${TESTSUITE_OUTPUT_DIR}")
    endif(WIN32)
    set(TEST_SUITE_COMMENT "Running the full FlightGear test suite: ${TEST_SUITE_WORKING_DIRECTORY}/${TEST_SUITE_COMMAND}")
else()
    set(TEST_SUITE_COMMENT "Building the FlightGear test suite.")
endif(ENABLE_AUTOTESTING)
set(TEST_SUITE_COMMAND
    ${TEST_SUITE_COMMAND}
    PARENT_SCOPE
)
set(TEST_SUITE_WORKING_DIRECTORY
    ${TEST_SUITE_WORKING_DIRECTORY}
    PARENT_SCOPE
)
set(TEST_SUITE_COMMENT
    ${TEST_SUITE_COMMENT}
    PARENT_SCOPE
)

# Set up the target links.
setup_fgfs_libraries(run_test_suite)

# Additional libraries just for the test suite.
target_link_libraries(run_test_suite ${CPPUNIT_LIBRARIES})
