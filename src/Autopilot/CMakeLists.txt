include(FlightGearComponent)

set(SOURCES
	analogcomponent.cxx
	autopilot.cxx
	autopilotgroup.cxx
	component.cxx
	digitalcomponent.cxx
	digitalfilter.cxx
	flipflop.cxx
	inputvalue.cxx
	logic.cxx
	pidcontroller.cxx
	pisimplecontroller.cxx
	predictor.cxx
	route_mgr.cxx
	)
	
flightgear_component(Autopilot "${SOURCES}")