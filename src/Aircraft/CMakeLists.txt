include(FlightGearComponent)

set(SOURCES
	controls.cxx
	replay.cxx
	)
	
flightgear_component(Aircraft "${SOURCES}")