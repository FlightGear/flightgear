include(FlightGearComponent)

set(SOURCES
	trafficcontrol.cxx
	CommStation.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
