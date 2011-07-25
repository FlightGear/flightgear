include(FlightGearComponent)

set(SOURCES
	ATC.cxx
	atis.cxx
	ATCDialogOld.cxx
	ATCVoice.cxx
	ATCmgr.cxx
	ATCutils.cxx
	ATCProjection.cxx
	)
	
flightgear_component(ATCDCL "${SOURCES}")
