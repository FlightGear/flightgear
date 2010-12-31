include(FlightGearComponent)

set(SOURCES
	atis_mgr.cxx
	trafficcontrol.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
