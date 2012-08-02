include(FlightGearComponent)

set(SOURCES
	NasalSys.cxx
	nasal-props.cxx
    NasalPositioned.cxx
    NasalCanvas.cxx
	)

set(HEADERS
	NasalSys.hxx
    NasalPositioned.hxx
    NasalCanvas.hxx
	)

	
flightgear_component(Scripting "${SOURCES}" "${HEADERS}")

