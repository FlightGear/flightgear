include(FlightGearComponent)

set(SOURCES
	ATC-Inputs.cxx
	ATC-Main.cxx
	ATC-Outputs.cxx
	AV400.cxx
	AV400Sim.cxx
	AV400WSim.cxx
	atlas.cxx
	garmin.cxx
	generic.cxx
	httpd.cxx
	joyclient.cxx
	jpg-httpd.cxx
	jsclient.cxx
	lfsglass.cxx
	native.cxx
	native_ctrls.cxx
	native_fdm.cxx
	native_gui.cxx
	nmea.cxx
	opengc.cxx
	props.cxx
	protocol.cxx
	pve.cxx
	ray.cxx
	rul.cxx
	)
	
flightgear_component(Network "${SOURCES}")

if(RTI_FOUND)
  add_subdirectory(HLA)
endif()

