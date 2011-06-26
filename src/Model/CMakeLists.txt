include(FlightGearComponent)

set(SOURCES
	acmodel.cxx
	model_panel.cxx
	modelmgr.cxx
	panelnode.cxx
	)

set(HEADERS
	acmodel.hxx
	model_panel.hxx
	modelmgr.hxx
	panelnode.hxx
	)
    		
flightgear_component(Model "${SOURCES}" "${HEADERS}")