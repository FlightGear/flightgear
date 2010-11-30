include(FlightGearComponent)

set(SOURCES
	electrical.cxx
	pitot.cxx
	static.cxx
	system_mgr.cxx
	vacuum.cxx
	)
	
flightgear_component(Systems "${SOURCES}")