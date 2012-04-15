include(FlightGearComponent)

set(SOURCES
	NasalSys.cxx
	nasal-props.cxx
    NasalPositioned.cxx
	)

set(HEADERS
	NasalSys.hxx
    NasalPositioned.hxx
	)

	
flightgear_component(Scripting "${SOURCES}" "${HEADERS}")

