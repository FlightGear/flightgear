include(FlightGearComponent)

set(SOURCES
	SchedFlight.cxx
	Schedule.cxx
	TrafficMgr.cxx
	)
	
flightgear_component(Traffic "${SOURCES}")