include(FlightGearComponent)

set(SOURCES
	TimeManager.cxx
	light.cxx
	bodysolver.cxx
	)
	
set(HEADERS
	TimeManager.hxx
	light.hxx
	bodysolver.hxx
	)
    	
flightgear_component(Time "${SOURCES}" "${HEADERS}")
