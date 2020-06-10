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


    add_library(CppUnitLib STATIC ${CPPUNIT_SOURCES} ${CPPUNIT_HEADERS})

    target_include_directories(CppUnitLib PUBLIC "${PROJECT_SOURCE_DIR}/3rdparty/cppunit/include")

    set(CPPUNIT_LIBRARIES CppUnitLib)
else()
    message(STATUS "CppUnit: Linking to the system supplied CppUnit library")
endif()

#-----------------------------------------------------------------------------
# Set up all test suites as CTests.

# Activation.
include(Dart)

# System test suites.
add_test(AeroMeshSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s AeroMeshTests)
#add_test(GPSSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s GPSTests)
#add_test(NavaidsSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s NavaidsTests)

# Unit test suites.
add_test(AddonManagementUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u AddonManagementTests)
add_test(AeroElementUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u AeroElementTests)
add_test(AircraftPerformanceUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u AircraftPerformanceTests)
add_test(AutosaveMigrationUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u AutosaveMigrationTests)
add_test(FlightplanUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u FlightplanTests)
add_test(FPNasalUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u FPNasalTests)
add_test(GPSUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u GPSTests)
add_test(HoldControllerUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u HoldControllerTests)
if(ENABLE_HID_INPUT)
    add_test(HIDInputUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u HIDInputTests)
endif()
add_test(LaRCSimMatrixUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u LaRCSimMatrixTests)
add_test(MktimeUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u MktimeTests)
add_test(NasalSysUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u NasalSysTests)
add_test(NavaidsUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u NavaidsTests)
add_test(NavRadioUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u NavRadioTests)
add_test(PosInitUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u PosInitTests)
add_test(RNAVProcedureUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u RNAVProcedureTests)
add_test(RouteManagerUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u RouteManagerTests)
add_test(YASimAtmosphereUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u YASimAtmosphereTests)

# GUI test suites.

# Simgear unit test suites.
add_test(CanvasSimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -m CanvasTests)
add_test(MathGeodesySimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -m MathGeodesyTests)
add_test(SimgearPropsSimgearUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -m SimgearPropsTests)

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

set_source_files_properties(${EMBEDDED_RESOURCE_SOURCES} PROPERTIES GENERATED TRUE)
set_source_files_properties(${EMBEDDED_RESOURCE_HEADERS} PROPERTIES GENERATED TRUE)

# Set up the separate executable for running the test suite.
add_executable(fgfs_test_suite
    ${FG_SOURCES}
    ${FG_HEADERS}
    ${EMBEDDED_RESOURCE_SOURCES}
    ${EMBEDDED_RESOURCE_HEADERS}
    ${TESTSUITE_SOURCES}
    ${TESTSUITE_HEADERS}
)

add_dependencies(fgfs_test_suite buildId embeddedresources)
set_target_properties(fgfs_test_suite
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TESTSUITE_OUTPUT_DIR}"
)
if(ENABLE_AUTOTESTING)
    set(TEST_SUITE_COMMAND "fgfs_test_suite")
    set(TEST_SUITE_COMMENT "Running the full FlightGear test suite")
else()
    set(TEST_SUITE_COMMENT "Building the FlightGear test suite.")
endif(ENABLE_AUTOTESTING)

# Set up the target links.
setup_fgfs_libraries(fgfs_test_suite)

target_compile_definitions(fgfs_test_suite PUBLIC BUILDING_TESTSUITE)

# Additional libraries just for the test suite.
target_link_libraries(fgfs_test_suite ${CPPUNIT_LIBRARIES})

# target to run the tests
if (MSVC)
    file(TO_NATIVE_PATH "${FG_QT_BIN_DIR}" _qt5_bin_dir_native)
    file(TO_NATIVE_PATH "${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" _install_bin_dir)

    set(CMAKE_MSVCIDE_RUN_PATH "${_install_bin_dir};${_msvc_3rdparty_bin_dir};${_qt5_bin_dir_native}")
endif()

if (FG_QT_ROOT_DIR)
    # for QtPlatformHeaders
    target_include_directories(fgfs_test_suite PRIVATE ${FG_QT_ROOT_DIR}/include)
endif()


add_custom_target(test_suite
    ${TEST_SUITE_COMMAND}
    DEPENDS fgfs_test_suite
    COMMENT ${TEST_SUITE_COMMENT}
)
