include(FlightGearComponent)

set(SOURCES
	atc_mgr.cxx
	atcdialog.cxx
	trafficcontrol.cxx
	CommStation.cxx
	)

set(HEADERS
	atc_mgr.hxx
	atcdialog.hxx
	trafficcontrol.hxx
	CommStation.hxx
	)
    	
flightgear_component(ATC "${SOURCES}" "${HEADERS}")
