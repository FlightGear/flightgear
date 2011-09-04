include(FlightGearComponent)

set(SOURCES
	atc_mgr.cxx
	atcdialog.cxx
	trafficcontrol.cxx
	CommStation.cxx
	itm.cpp
	)
	
flightgear_component(ATC "${SOURCES}")
