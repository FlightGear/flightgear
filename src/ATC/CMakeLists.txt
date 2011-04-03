include(FlightGearComponent)

set(SOURCES
	atis_mgr.cxx
	atc_mgr.cxx
	trafficcontrol.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
