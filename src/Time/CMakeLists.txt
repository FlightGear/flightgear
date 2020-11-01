include(FlightGearComponent)

set(SOURCES
	TimeManager.cxx
	light.cxx
        tide.cxx
	bodysolver.cxx
	)
	
set(HEADERS
	TimeManager.hxx
	light.hxx
        tide.hxx
	bodysolver.hxx
	)
    	
flightgear_component(Time "${SOURCES}" "${HEADERS}")
