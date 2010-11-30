include(FlightGearComponent)

set(SOURCES
	SceneryPager.cxx
	redout.cxx
	scenery.cxx
	tilemgr.cxx
	)
	
flightgear_component(Scenery "${SOURCES}")

