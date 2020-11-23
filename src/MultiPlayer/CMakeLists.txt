include(FlightGearComponent)

set(SOURCES
	multiplaymgr.cxx
	tiny_xdr.cxx
	MPServerResolver.cxx
	mpirc.cxx
	cpdlc.cxx
	)

set(HEADERS
	multiplaymgr.hxx
	tiny_xdr.hxx
	MPServerResolver.hxx
	mpirc.hxx
	cpdlc.hxx
	)
    	
flightgear_component(MultiPlayer "${SOURCES}" "${HEADERS}")
