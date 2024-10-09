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
elseif(NOT TARGET CppUnitLib)
    message(FATAL_ERROR "CppUnit: system CppUnit library not found correctly")
else()
    message(STATUS "CppUnit: Linking to the system supplied CppUnit library")
endif()

#-----------------------------------------------------------------------------
# Set up all test suites as CTests.

# System test suites.
add_test(AeroMeshSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s AeroMeshTests)
#add_test(GPSSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s GPSTests)
add_test(InstancedSubsystemSystemTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -s InstancedSubsystemTests)
#add_test(NavaidsSystemTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -s NavaidsTests)
add_test(NonInstancedSubsystemSystemTests ${TESTSUITE_OUTPUT_DIR}/run_test_suite --ctest -s NonInstancedSubsystemTests)

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
add_test(NasalSysUnitTests ${TESTSUITE_OUTPUT_DIR}/fgfs_test_suite --ctest -u NasalLibTests)
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

# most object files come from fgfsObjects target: this property
# contains all the sources which need to be compiled seperately
# for the test-suite, becuase they depend on BUILDING_TESTSUTIE define
# we setup below via target_compile_definitions

get_property(TEST_SOURCES GLOBAL PROPERTY FG_TEST_SOURCES)

# Set up the separate executable for running the test suite.
add_executable(fgfs_test_suite
    ${TEST_SOURCES}
    ${TESTSUITE_SOURCES}
    ${TESTSUITE_HEADERS}
    $<TARGET_OBJECTS:fgfsObjects>
)

add_dependencies(fgfs_test_suite buildId)
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
# Additional search paths for includes.
setup_fgfs_includes(fgfs_test_suite)

# test codce includes its headers as test_suite/foo.h, so we need this
target_include_directories(fgfs_test_suite PUBLIC ${PROJECT_SOURCE_DIR})

target_compile_definitions(fgfs_test_suite PUBLIC BUILDING_TESTSUITE)

# Additional libraries just for the test suite.
target_link_libraries(fgfs_test_suite CppUnitLib)

# target to run the tests
if (MSVC)
    file(TO_NATIVE_PATH "${FG_QT_BIN_DIR}" _qt_bin_dir_native)
    file(TO_NATIVE_PATH "${FINAL_MSVC_3RDPARTY_DIR}/bin" _msvc_3rdparty_bin_dir)
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" _install_bin_dir)

    set(CMAKE_MSVCIDE_RUN_PATH "${_install_bin_dir};${_msvc_3rdparty_bin_dir};${_qt_bin_dir_native}")
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
