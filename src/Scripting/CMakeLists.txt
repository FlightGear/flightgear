include(FlightGearComponent)

set(SOURCES
	NasalSys.cxx
	nasal-props.cxx
	)

set(HEADERS
	NasalSys.hxx
	)

	
flightgear_component(Scripting "${SOURCES}" "${HEADERS}")

