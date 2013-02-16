include(FlightGearComponent)

set(SOURCES
	SceneryPager.cxx
	redout.cxx
	scenery.cxx
	tilecache.cxx
	tileentry.cxx
	tilemgr.cxx
	)

set(HEADERS
	SceneryPager.hxx
	redout.hxx
	scenery.hxx
	tilecache.hxx
	tileentry.hxx
	tilemgr.hxx
	)

flightgear_component(Scenery "${SOURCES}" "${HEADERS}")
