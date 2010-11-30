include(FlightGearComponent)

set(SOURCES
	atcutils.cxx
	atis.cxx
	trafficcontrol.cxx
	)
	
flightgear_component(ATC "${SOURCES}")