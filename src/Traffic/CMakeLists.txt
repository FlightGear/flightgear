include(FlightGearComponent)

set(SOURCES
	SchedFlight.cxx
	Schedule.cxx
	TrafficMgr.cxx
	)

set(HEADERS
	SchedFlight.hxx
	Schedule.hxx
	TrafficMgr.hxx
)


flightgear_component(Traffic "${SOURCES}" "${HEADERS}")