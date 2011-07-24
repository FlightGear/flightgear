include(FlightGearComponent)

set(SOURCES
	beacon.cxx
	fg_fx.cxx
	morse.cxx
	sample_queue.cxx
	voice.cxx
	voiceplayer.cxx
	)

set(HEADERS
	beacon.hxx
	fg_fx.hxx
	morse.hxx
	sample_queue.hxx
	voice.hxx
	voiceplayer.hxx
	)
    		
flightgear_component(Sound "${SOURCES}" "${HEADERS}")