include(FlightGearComponent)

set(SOURCES
	AIAircraft.cxx
	AIBallistic.cxx
	AIBase.cxx
	AICarrier.cxx
	AIEscort.cxx
	AIFlightPlan.cxx
	AIFlightPlanCreate.cxx
	AIFlightPlanCreateCruise.cxx
	AIFlightPlanCreatePushBack.cxx
	AIGroundVehicle.cxx
	AIManager.cxx
	AIMultiplayer.cxx
	AIShip.cxx
	AIStatic.cxx
	AIStorm.cxx
	AITanker.cxx
	AIThermal.cxx
	AIWingman.cxx
	performancedata.cxx
	performancedb.cxx
	submodel.cxx
	)
	
flightgear_component(AIModel "${SOURCES}")
	