include(FlightGearComponent)

set(SOURCES
	acmodel.cxx
	modelmgr.cxx
	panelnode.cxx
	)

set(HEADERS
	acmodel.hxx
	modelmgr.hxx
	panelnode.hxx
	)
    		
flightgear_component(Model "${SOURCES}" "${HEADERS}")