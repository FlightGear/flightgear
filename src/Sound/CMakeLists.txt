include(FlightGearComponent)

set(SOURCES
	audioident.cxx
	soundgenerator.cxx
	beacon.cxx
	fg_fx.cxx
	morse.cxx
	sample_queue.cxx
	voice.cxx
	voiceplayer.cxx
	soundmanager.cxx
	
	)

set(HEADERS
	audioident.hxx
	soundgenerator.hxx
	beacon.hxx
	fg_fx.hxx
	morse.hxx
	sample_queue.hxx
	voice.hxx
	voiceplayer.hxx
	soundmanager.hxx
	VoiceSynthesizer.hxx
	flitevoice.hxx
	)

flightgear_component(Sound "${SOURCES}" "${HEADERS}")

add_library(fgvoicesynth STATIC VoiceSynthesizer.cxx flitevoice.cxx)
target_link_libraries(fgvoicesynth PRIVATE flightgear_flite_hts SimGearScene)
