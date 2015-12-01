
include_directories(${PROJECT_SOURCE_DIR})

# note order here affects link order, and hence linking correctness
# on systems with a traditional ld (eg, GNU ld on Linux)
foreach( mylibfolder
		Airports
		Aircraft
		ATC
		Canvas
		Radio
		Autopilot
		Cockpit
		Environment
		GUI
		Input
		Instrumentation
		Model
		MultiPlayer
		AIModel
		Navaids
		Network
		Scenery
		Scripting
		Sound
		Systems
		Time
		Traffic
		FDM
		Viewer
		Main
	)

	add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

add_library(fgcorelib
                Autopilot/route_mgr.cxx
		Airports/airport.cxx
		Airports/apt_loader.cxx
		Airports/airportdynamicsmanager.cxx
		Airports/gnnode.cxx
		Airports/parking.cxx
		Airports/dynamics.cxx
		Airports/groundnetwork.cxx
		Airports/dynamicloader.cxx
		Airports/xmlloader.cxx
		Airports/runwayprefs.cxx
		Airports/runwayprefloader.cxx
		Airports/runwaybase.cxx
		Airports/runways.cxx
		Airports/pavement.cxx
		ATC/CommStation.cxx
		Environment/environment.cxx
		Environment/atmosphere.cxx
		Navaids/NavDataCache.cxx
		Navaids/positioned.cxx
		Navaids/PositionedOctree.cxx
		Navaids/airways.cxx
		Navaids/navdb.cxx
		Navaids/navlist.cxx
		Navaids/navrecord.cxx
		Navaids/fixlist.cxx
		Navaids/poidb.cxx
		Navaids/markerbeacon.cxx
		Navaids/route.cxx
		Navaids/waypoint.cxx
		Navaids/LevelDXML.cxx
		Navaids/procedure.cxx
                Navaids/FlightPlan.cxx
                Navaids/routePath.cxx
		Main/globals.cxx
		Main/fg_props.cxx
		Main/locale.cxx
		Main/testStubs.cxx
		Main/testStubs.hxx
)

set_target_properties(fgcorelib PROPERTIES COMPILE_DEFINITIONS FG_TEST_LIB)

target_link_libraries(fgcorelib
	${SQLITE3_LIBRARY}
	${SIMGEAR_LIBRARIES}
	${ZLIB_LIBRARY}
	${EVENT_INPUT_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${PLATFORM_LIBS}
)
