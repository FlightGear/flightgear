include(FlightGearComponent)

set(SOURCES
	multiplaymgr.cxx
	tiny_xdr.cxx
	)

flightgear_component(MultiPlayer "${SOURCES}")