include(FlightGearComponent)

set(SOURCES
	atc_mgr.cxx
	trafficcontrol.cxx
	CommStation.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
