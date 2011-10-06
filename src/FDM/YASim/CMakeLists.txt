include(FlightGearComponent)

set(SOURCES
    Airplane.cpp
	Atmosphere.cpp
	ControlMap.cpp
	FGFDM.cpp
	FGGround.cpp
	Gear.cpp
	Glue.cpp
	Ground.cpp
	Hitch.cpp
	Hook.cpp
	Integrator.cpp
	Jet.cpp
	Launchbar.cpp
	Math.cpp
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
	YASim.cxx
    )

flightgear_component(YASim "${SOURCES}")
    
add_executable(yasim yasim-test.cpp)

target_link_libraries(yasim 
	fgYASim
	${SIMGEAR_CORE_LIBRARIES}
        ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})

install(TARGETS yasim RUNTIME DESTINATION bin)
	

