include(FlightGearComponent)

set(SOURCES
	multiplaymgr.cxx
	tiny_xdr.cxx
	)

set(HEADERS
	multiplaymgr.hxx
	tiny_xdr.hxx
	)
    	
flightgear_component(MultiPlayer "${SOURCES}" "${HEADERS}")