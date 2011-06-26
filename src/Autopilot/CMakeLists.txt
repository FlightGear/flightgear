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
	
set(HEADERS
	analogcomponent.hxx
	autopilot.hxx
	autopilotgroup.hxx
	component.hxx
	digitalcomponent.hxx
	digitalfilter.hxx
	flipflop.hxx
	inputvalue.hxx
	logic.hxx
	pidcontroller.hxx
	pisimplecontroller.hxx
	predictor.hxx
	route_mgr.hxx
	)
    	
flightgear_component(Autopilot "${SOURCES}" "${HEADERS}")