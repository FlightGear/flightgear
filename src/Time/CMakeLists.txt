include(FlightGearComponent)

set(SOURCES
	TimeManager.cxx
	light.cxx
	sunsolver.cxx
	)
	
set(HEADERS
	TimeManager.hxx
	light.hxx
	sunsolver.hxx
	)
    	
flightgear_component(Time "${SOURCES}" "${HEADERS}")