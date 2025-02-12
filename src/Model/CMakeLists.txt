include(FlightGearComponent)

set(SOURCES
	acmodel.cxx
	modelmgr.cxx
	)

set(HEADERS
	acmodel.hxx
	modelmgr.hxx
	)
    		
flightgear_component(Model "${SOURCES}" "${HEADERS}")
