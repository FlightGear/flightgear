include(FlightGearComponent)

set(SOURCES
	acmodel.cxx
	model_panel.cxx
	modelmgr.cxx
	panelnode.cxx
	)
	
flightgear_component(Model "${SOURCES}")