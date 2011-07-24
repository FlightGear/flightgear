include(FlightGearComponent)

set(SOURCES
	SceneryPager.cxx
	redout.cxx
	scenery.cxx
	tilemgr.cxx
	)

set(HEADERS
	SceneryPager.hxx
	redout.hxx
	scenery.hxx
	tilemgr.hxx
	)
    		
flightgear_component(Scenery "${SOURCES}" "${HEADERS}")

