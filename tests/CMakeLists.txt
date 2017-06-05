
set(sources
  Main/options.cxx
  Main/fg_commands.cxx
  Main/fg_props.cxx
  Main/globals.cxx
  Main/locale.cxx
  Main/util.cxx
  Main/positioninit.cxx
  Aircraft/controls.cxx
  Aircraft/FlightHistory.cxx
  Aircraft/flightrecorder.cxx
  Aircraft/replay.cxx
  Autopilot/route_mgr.cxx
  Airports/airport.cxx
  Airports/airport.hxx
  Airports/apt_loader.cxx
  Airports/airportdynamicsmanager.cxx
  Airports/airportdynamicsmanager.hxx
  Airports/dynamicloader.cxx
  Airports/dynamics.cxx
  Airports/xmlloader.cxx
  Airports/runwaybase.cxx
  Airports/pavement.cxx
  Airports/parking.cxx
  Airports/groundnetwork.cxx
  Airports/gnnode.cxx
  Airports/runways.cxx
  Airports/runwayprefs.cxx
  Airports/runwayprefloader.cxx
  ATC/CommStation.cxx
#  ATC/GroundController.cxx
#  ATC/atc_mgr.cxx
  Environment/atmosphere.cxx
  Environment/environment.cxx
  Environment/environment_mgr.cxx
  Environment/environment_ctrl.cxx
  Environment/presets.cxx
  Environment/gravity.cxx
  Environment/ridge_lift.cxx
  Environment/magvarmanager.cxx
  Navaids/airways.cxx
  Navaids/fixlist.cxx
  Navaids/markerbeacon.cxx
  Navaids/NavDataCache.cxx
  Navaids/navdb.cxx
  Navaids/navlist.cxx
  Navaids/navrecord.cxx
  Navaids/poidb.cxx
  Navaids/procedure.cxx
  Navaids/positioned.cxx
  Navaids/PositionedOctree.cxx
  Navaids/routePath.cxx
  Navaids/route.cxx
  Navaids/waypoint.cxx
  Navaids/FlightPlan.cxx
  Navaids/LevelDXML.cxx
  Network/HTTPClient.cxx
  Time/TimeManager.cxx
  Time/bodysolver.cxx
  Scripting/NasalSys.cxx
  Scripting/NasalCondition.cxx
  Scripting/NasalAircraft.cxx
  Scripting/NasalString.cxx
  Scripting/NasalPositioned.cxx
  Scripting/NasalPositioned_cppbind.cxx
  Scripting/nasal-props.cxx
  Scripting/NasalSGPath.cxx
  Scripting/NasalHTTP.cxx
  Viewer/view.cxx
  Viewer/viewmgr.cxx
)

foreach(s ${sources})
  set_property(DIRECTORY APPEND PROPERTY fgtestlib_sources "${CMAKE_SOURCE_DIR}/src/${s}")
endforeach()

set_property(DIRECTORY APPEND PROPERTY fgtestlib_sources "${CMAKE_SOURCE_DIR}/3rdparty/cjson/cJSON.c")

get_property(fgtestlib_sources DIRECTORY PROPERTY fgtestlib_sources)

add_library(fgtestlib SHARED ${fgtestlib_sources}
  unitTestHelpers.cxx
  testStubs.cxx
  fake_sgSky.cxx
  fake_sgPrecipitation.cxx
  fake_sound.cxx)

set_target_properties (fgtestlib
  PROPERTIES
  COMPILE_DEFINITIONS "FG_TESTLIB"
)

if(FG_HAVE_GPERFTOOLS)
    target_include_directories(fgtestlib PRIVATE ${GooglePerfTools_INCLUDE_DIR})
    target_link_libraries(fgtestlib ${GooglePerfTools_LIBRARIES})
endif()
target_link_libraries(fgtestlib SimGearCore ${PLATFORM_LIBS} ${SQLITE3_LIBRARY})

add_executable(fgtest fgTestDriver.cxx)
target_link_libraries(fgtest fgtestlib)

macro(flightgear_test name sources)
  add_executable(${name} ${sources})
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
  target_link_libraries(${name} fgtestlib)
  add_test(${name} ${EXECUTABLE_OUTPUT_PATH}/${name})
endmacro()

flightgear_test(test_navs test_navaids2.cxx)
flightgear_test(test_flightplan test_flightplan.cxx)

add_executable(test_ls_matrix test_ls_matrix.cxx ${CMAKE_SOURCE_DIR}/src/FDM/LaRCsim/ls_matrix.c)
target_link_libraries(test_ls_matrix SimGearCore)
add_test(test_ls_matrix ${EXECUTABLE_OUTPUT_PATH}/test_ls_matrix)
