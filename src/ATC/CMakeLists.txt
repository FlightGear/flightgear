include(FlightGearComponent)

set(SOURCES
	atc_mgr.cxx
	atcdialog.cxx
	trafficcontrol.cxx
	CommStation.cxx
	)
	
flightgear_component(ATC "${SOURCES}")
