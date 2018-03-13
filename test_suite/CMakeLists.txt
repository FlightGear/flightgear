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
# From here on, this is a modified copy of src/Main/CMakeLists.txt.  This is
# designed to have a minimal diff so that this can be updated together with the
# main CMakeLists.txt file.

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
    ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
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
    ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
)

# On Windows, make sure fgrcc can be run (it needs third-party libraries)
if(MSVC)
  if(MSVC_3RDPARTY_ROOT AND MSVC_3RDPARTY_DIR)
    set(CMAKE_MSVCIDE_RUN_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/bin)
  else()
    message(FATAL_ERROR
      "Either MSVC_3RDPARTY_ROOT or MSVC_3RDPARTY_DIR is empty or unset")
  endif()
endif()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx
         ${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx
  COMMAND fgrcc --root=${CMAKE_SOURCE_DIR} --output-cpp-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.cxx --init-func-name=initFlightGearEmbeddedResources --output-header-file=${CMAKE_BINARY_DIR}/src/EmbeddedResources/FlightGear-resources.hxx --output-header-identifier=_FG_FLIGHTGEAR_EMBEDDED_RESOURCES ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
  DEPENDS
    fgrcc ${CMAKE_SOURCE_DIR}/src/EmbeddedResources/FlightGear-resources.xml
)

# Get the FG sources and headers to be built into the test suite.
get_property(FG_SOURCES GLOBAL PROPERTY FG_SOURCES)
get_property(FG_HEADERS GLOBAL PROPERTY FG_HEADERS)

link_directories ( ${Boost_LIBRARY_DIRS} )

# Set up the source groups.
get_property(FG_GROUPS_C GLOBAL PROPERTY FG_GROUPS_C)
string(REPLACE "@" ";" groups ${FG_GROUPS_C} )
foreach(g ${groups})
    string(REPLACE "#" ";" g2 ${g})
    list(GET g2 0 name)
    list(REMOVE_AT g2 0)
    source_group("${name}\\Sources" FILES ${g2})
endforeach()

# Set up the header groups.
get_property(FG_GROUPS_H GLOBAL PROPERTY FG_GROUPS_H)
string(REPLACE "@" ";" groups ${FG_GROUPS_H} )
foreach(g ${groups})
    string(REPLACE "#" ";" g2 ${g})
    list(GET g2 0 name)
    list(REMOVE_AT g2 0)
    source_group("${name}\\Headers" FILES ${g2})
endforeach()

source_group("Main\\Headers" FILES ${HEADERS})
source_group("Main\\Sources" FILES ${SOURCES})

# Set up the separate executable for running the test suite.
add_executable(run_test_suite
    ${FG_SOURCES}
    ${FG_HEADERS}
    ${MAIN_SOURCES}
    ${MAIN_HEADERS}
    ${TESTSUITE_SOURCES}
    ${TESTSUITE_HEADERS}
)
set_target_properties(run_test_suite
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TESTSUITE_OUTPUT_DIR}"
)
if(WIN32)
    add_custom_target(test_suite "run_test_suite.exe"
        DEPENDS run_test_suite
        WORKING_DIRECTORY "${TESTSUITE_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}"
        COMMENT "Running the full FlightGear test-suite."
    )
else()
    add_custom_target(test_suite "${TESTSUITE_OUTPUT_DIR}/run_test_suite"
        DEPENDS run_test_suite
        COMMENT "Running the full FlightGear test-suite."
    )
endif(WIN32)

#-----------------------------------------------------------------------------

get_property(FG_LIBS GLOBAL PROPERTY FG_LIBS)
#message(STATUS "fg libs ${FG_LIBS}")
#message(STATUS "OSG libs ${OPENSCENEGRAPH_LIBRARIES}")
#message(STATUS "SG libs ${SIMGEAR_LIBRARIES}")

if(RTI_FOUND)
    set(HLA_LIBRARIES ${RTI_LIBRARIES})
else()
    set(HLA_LIBRARIES "")
endif()

if(GDAL_FOUND)
    set(GDAL_LIBRARIES ${GDAL_LIBRARY})
else()
    set(GDAL_LIBRARIES "")
endif()

if(ENABLE_JSBSIM)
    # FIXME - remove once JSBSim doesn't expose private headers
    include_directories(${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)

    target_link_libraries(run_test_suite JSBSim)
endif()

if(ENABLE_IAX)
    target_link_libraries(run_test_suite iaxclient_lib ${OPENAL_LIBRARY})
endif()

if(USE_DBUS)
    target_link_libraries(run_test_suite ${DBUS_LIBRARIES})
endif()

if(FG_HAVE_GPERFTOOLS)
    include_directories(${GooglePerfTools_INCLUDE_DIR})
    target_link_libraries(run_test_suite ${GooglePerfTools_LIBRARIES})
endif()

if(CRASHRPT_FOUND)
    target_link_libraries(run_test_suite ${CRASHRPT_LIBRARY})
endif()

if(X11_FOUND)
    target_link_libraries(run_test_suite ${X11_LIBRARIES})
endif()

target_link_libraries(run_test_suite
    SimGearCore
    SimGearScene
    ${CPPUNIT_LIBRARIES}
    ${EVENT_INPUT_LIBRARIES}
    ${GDAL_LIBRARIES}
    ${HLA_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${PLATFORM_LIBS}
    ${PLIB_LIBRARIES}
    ${SQLITE3_LIBRARY}
    ${SIMGEAR_LIBRARIES}
)


if(ENABLE_FLITE)
    if(SYSTEM_HTS_ENGINE)
        target_link_libraries(run_test_suite flite_hts ${HTS_ENGINE_LIBRARIES})
    else()
        target_link_libraries(run_test_suite flite_hts hts_engine)
    endif()
endif()

if(Qt5Core_FOUND)
    target_link_libraries(run_test_suite Qt5::Core Qt5::Widgets fglauncher fgqmlui)
    set_property(TARGET run_test_suite PROPERTY AUTOMOC ON)
endif()

if(USE_AEONWAVE)
   target_link_libraries(run_test_suite ${AAX_LIBRARY})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries(run_test_suite execinfo)
endif()
