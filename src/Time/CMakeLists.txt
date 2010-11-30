include(FlightGearComponent)

set(SOURCES
	TimeManager.cxx
	light.cxx
	sunsolver.cxx
	)
	
flightgear_component(Time "${SOURCES}")