include(FlightGearComponent)

set(SOURCES
	beacon.cxx
	fg_fx.cxx
	morse.cxx
	sample_queue.cxx
	voice.cxx
	)
	
flightgear_component(Sound "${SOURCES}")