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
	HTTPClient.cxx
	joyclient.cxx
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

set(HEADERS
	ATC-Inputs.hxx
	ATC-Main.hxx
	ATC-Outputs.hxx
	AV400.hxx
	AV400Sim.hxx
	AV400WSim.hxx
	atlas.hxx
	garmin.hxx
	generic.hxx
	httpd.hxx
	HTTPClient.hxx
	joyclient.hxx
	jsclient.hxx
	lfsglass.hxx
	native.hxx
	native_ctrls.hxx
	native_fdm.hxx
	native_gui.hxx
	nmea.hxx
	opengc.hxx
	props.hxx
	protocol.hxx
	pve.hxx
	ray.hxx
	rul.hxx
	)
	
if(FG_JPEG_SERVER)
    list(APPEND SOURCES jpg-httpd.cxx)
    list(APPEND HEADERS jpg-httpd.hxx)
endif()
   	
flightgear_component(Network "${SOURCES}" "${HEADERS}")

if(RTI_FOUND)
  add_subdirectory(HLA)
endif()

