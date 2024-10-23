include(FlightGearComponent)

if(SP_FDMS)
set(SP_FDM_SOURCES
	SP/ACMS.cxx
	SP/ADA.cxx
	SP/Balloon.cxx
	SP/BalloonSim.cpp
	SP/MagicCarpet.cxx
	SP/AISim.cpp
	)
set (SP_FDM_HEADERS
	SP/ACMS.hxx
	SP/ADA.hxx
	SP/MagicCarpet.hxx
	SP/Balloon.h
	SP/BalloonSim.h
	SP/AISim.hpp
	)
endif()
	
set(SOURCES
	NullFDM.cxx
	UFO.cxx
	fdm_shell.cxx
	flight.cxx
	flightProperties.cxx
	TankProperties.cxx
	groundcache.cxx
     groundreactions.cxx
	${SP_FDM_SOURCES}
	ExternalNet/ExternalNet.cxx
	ExternalPipe/ExternalPipe.cxx
	AIWake/AircraftMesh.cxx
	AIWake/WakeMesh.cxx
	AIWake/AeroElement.cxx
	AIWake/AIWakeGroup.cxx
	ls_matrix.c
	)

set(HEADERS
	NullFDM.hxx
	TankProperties.hxx
	UFO.hxx
	fdm_shell.hxx
	flight.hxx
	flightProperties.hxx
	groundcache.hxx
    groundreactions.hxx
	${SP_FDM_HEADERS}
	ls_matrix.h
	)

flightgear_component(FDM "${SOURCES}" "${HEADERS}")

if(ENABLE_YASIM)
	add_subdirectory(YASim)
endif()


if(ENABLE_JSBSIM)
    add_subdirectory(JSBSim)
endif()

