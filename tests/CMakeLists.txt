
set(sources
  Main/options.cxx
  Main/fg_commands.cxx
  Main/fg_props.cxx
  Main/globals.cxx
  Main/locale.cxx
  Main/util.cxx
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

target_link_libraries(fgtestlib SimGearCore ${PLATFORM_LIBS})

add_executable(fgtest fgTestDriver.cxx)
target_link_libraries(fgtest fgtestlib)

# repeat this section for each unit-test executable
add_executable(testnavs test_navaids2.cxx)
target_link_libraries(testnavs fgtestlib fgsqlite3)
add_test(testnavs ${EXECUTABLE_OUTPUT_PATH}/testnavs)

add_executable(testflightplan test_flightplan.cxx)
target_link_libraries(testflightplan fgtestlib fgsqlite3)
add_test(testflightplan ${EXECUTABLE_OUTPUT_PATH}/testflightplan)
