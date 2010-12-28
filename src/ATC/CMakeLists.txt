include(FlightGearComponent)

set(SOURCES
	atis.cxx
	trafficcontrol.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
