include(FlightGearComponent)

set(SOURCES
	electrical.cxx
	pitot.cxx
	static.cxx
	system_mgr.cxx
	vacuum.cxx
	)

set(HEADERS
	electrical.hxx
	pitot.hxx
	static.hxx
	system_mgr.hxx
	vacuum.hxx
	)

	
flightgear_component(Systems "${SOURCES}" "${HEADERS}")