include(FlightGearComponent)

set(SOURCES
	multiplaymgr.cxx
	tiny_xdr.cxx
        MPServerResolver.cxx
	)

set(HEADERS
	multiplaymgr.hxx
	tiny_xdr.hxx
        MPServerResolver.hxx
	)
    	
flightgear_component(MultiPlayer "${SOURCES}" "${HEADERS}")
