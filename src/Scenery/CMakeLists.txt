include(FlightGearComponent)

set(SOURCES
	SceneryPager.cxx
	redout.cxx
	scenery.cxx
	terrain_stg.cxx
	terrain_spt.cxx
	tilecache.cxx
	tileentry.cxx
	tilemgr.cxx
	)

set(HEADERS
	SceneryPager.hxx
	redout.hxx
	scenery.hxx
	terrain.hxx
	terrain_stg.hxx
	terrain_spt.hxx
	tilecache.hxx
	tileentry.hxx
	tilemgr.hxx
	)

flightgear_component(Scenery "${SOURCES}" "${HEADERS}")
