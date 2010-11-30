include(FlightGearComponent)

set(SOURCES
	NasalSys.cxx
	nasal-props.cxx
	)
	
flightgear_component(Scripting "${SOURCES}")

