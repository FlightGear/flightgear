include(FlightGearComponent)

set(COMMON
	Airplane.cpp
	Atmosphere.cpp
	ControlMap.cpp
	FGFDM.cpp
	Gear.cpp
	Glue.cpp
	Ground.cpp
	Hitch.cpp
	Hook.cpp
	Integrator.cpp
	Jet.cpp
	Launchbar.cpp
	Model.cpp
	PistonEngine.cpp
	PropEngine.cpp
	Propeller.cpp
	RigidBody.cpp
	Rotor.cpp
	Rotorpart.cpp
	SimpleJet.cpp
	Surface.cpp
	Thruster.cpp
	TurbineEngine.cpp
	Turbulence.cpp
	Wing.cpp
	)

set(SOURCES
	${COMMON}
	YASim.cxx
	FGGround.cpp
	)

flightgear_component(YASim  "${SOURCES}")

if(ENABLE_TESTS)
add_executable(yasim yasim-test.cpp ${COMMON})
add_executable(yasim-proptest proptest.cpp ${COMMON})

target_link_libraries(yasim
		${SIMGEAR_CORE_LIBRARIES}
		${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})

target_link_libraries(yasim-proptest
		${SIMGEAR_CORE_LIBRARIES}
		${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})

install(TARGETS yasim yasim-proptest RUNTIME DESTINATION bin)

endif(ENABLE_TESTS)

