include(FlightGearComponent)

set(SOURCES
	ATCProjection.cxx
	)

set(HEADERS
	ATCProjection.hxx
	)
    	
flightgear_component(ATCDCL "${SOURCES}" "${HEADERS}")
