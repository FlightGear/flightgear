include(FlightGearComponent)

set(HEADERS
    FGFDMExec.h
    FGJSBBase.h
    JSBSim.hxx
    initialization/FGInitialCondition.h
    initialization/FGTrim.h
    initialization/FGTrimAxis.h
    input_output/FGXMLParse.h
    input_output/FGXMLFileRead.h
    input_output/FGPropertyReader.h
    input_output/FGPropertyManager.h
    input_output/FGScript.h
    input_output/FGfdmSocket.h
    input_output/string_utilities.h
    input_output/FGXMLElement.h
    input_output/net_fdm.hxx
    input_output/FGGroundCallback.h
    input_output/FGInputType.h
    input_output/FGInputSocket.h
    input_output/FGUDPInputSocket.h
    input_output/FGOutputFG.h
    input_output/FGOutputFile.h
    input_output/FGOutputSocket.h
    input_output/FGOutputTextFile.h
    input_output/FGOutputType.h
    input_output/FGModelLoader.h
    math/FGParameter.h
    math/LagrangeMultiplier.h
    math/FGColumnVector3.h
    math/FGCondition.h
    math/FGFunction.h
    math/FGLocation.h
    math/FGMatrix33.h
    math/FGModelFunctions.h
    math/FGPropertyValue.h
    math/FGQuaternion.h
    math/FGRealValue.h
    math/FGRungeKutta.h
    math/FGTable.h
    math/FGFunctionValue.h
    math/FGTemplateFunc.h
    models/FGAccelerations.h
    models/FGAerodynamics.h
    models/FGAircraft.h
    models/FGAtmosphere.h
    models/FGAuxiliary.h
    models/FGBuoyantForces.h
    models/FGExternalForce.h
    models/FGExternalReactions.h
    models/FGFCS.h
    models/FGGasCell.h
    models/FGSurface.h
    models/FGGroundReactions.h
    models/FGInertial.h
    models/FGInput.h
    models/FGLGear.h
    models/FGMassBalance.h
    models/FGModel.h
    models/FGOutput.h
    models/FGPropagate.h
    models/FGPropulsion.h
    models/atmosphere/FGStandardAtmosphere.h
    models/atmosphere/FGWinds.h
    models/flight_control/FGAccelerometer.h
    models/flight_control/FGActuator.h
    models/flight_control/FGAngles.h
    models/flight_control/FGDeadBand.h
    models/flight_control/FGFCSComponent.h
    models/flight_control/FGFCSFunction.h
    models/flight_control/FGFilter.h
    models/flight_control/FGGain.h
    models/flight_control/FGGyro.h
    models/flight_control/FGKinemat.h
    models/flight_control/FGMagnetometer.h
    models/flight_control/FGPID.h
    models/flight_control/FGSensor.h
    models/flight_control/FGSensorOrientation.h
    models/flight_control/FGSummer.h
    models/flight_control/FGSwitch.h
    models/flight_control/FGWaypoint.h
    models/flight_control/FGDistributor.h
    models/flight_control/FGLinearActuator.h
    models/propulsion/FGElectric.h
    models/propulsion/FGEngine.h
    models/propulsion/FGForce.h
    models/propulsion/FGNozzle.h
    models/propulsion/FGPiston.h
    models/propulsion/FGPropeller.h
    models/propulsion/FGRocket.h
    models/propulsion/FGRotor.h
    models/propulsion/FGTank.h
    models/propulsion/FGThruster.h
    models/propulsion/FGTransmission.h
    models/propulsion/FGTurbine.h
    models/propulsion/FGTurboProp.h
    )

set(SOURCES
    FGFDMExec.cpp
    FGJSBBase.cpp
    JSBSim.cxx
    initialization/FGInitialCondition.cpp
    initialization/FGTrim.cpp
    initialization/FGTrimAxis.cpp
    input_output/FGGroundCallback.cpp
    input_output/FGPropertyReader.cpp
    input_output/FGPropertyManager.cpp
    input_output/FGScript.cpp
    input_output/FGXMLElement.cpp
    input_output/FGXMLParse.cpp
    input_output/FGfdmSocket.cpp
    input_output/FGInputType.cpp
    input_output/FGInputSocket.cpp
    input_output/FGUDPInputSocket.cpp
    input_output/FGOutputFG.cpp
    input_output/FGOutputFile.cpp
    input_output/FGOutputSocket.cpp
    input_output/FGOutputTextFile.cpp
    input_output/FGOutputType.cpp
    input_output/FGModelLoader.cpp
    math/FGColumnVector3.cpp
    math/FGCondition.cpp
    math/FGFunction.cpp
    math/FGLocation.cpp
    math/FGMatrix33.cpp
    math/FGModelFunctions.cpp
    math/FGPropertyValue.cpp
    math/FGQuaternion.cpp
    math/FGRealValue.cpp
    math/FGRungeKutta.cpp
    math/FGTable.cpp
    math/FGTemplateFunc.cpp
    models/FGAccelerations.cpp
    models/FGAerodynamics.cpp
    models/FGAircraft.cpp
    models/FGAtmosphere.cpp
    models/FGAuxiliary.cpp
    models/FGBuoyantForces.cpp
    models/FGExternalForce.cpp
    models/FGExternalReactions.cpp
    models/FGFCS.cpp
    models/FGGasCell.cpp
    models/FGSurface.cpp
    models/FGGroundReactions.cpp
    models/FGInertial.cpp
    models/FGInput.cpp
    models/FGLGear.cpp
    models/FGMassBalance.cpp
    models/FGModel.cpp
    models/FGOutput.cpp
    models/FGPropagate.cpp
    models/FGPropulsion.cpp
    models/atmosphere/FGStandardAtmosphere.cpp
    models/atmosphere/FGWinds.cpp
    models/flight_control/FGAccelerometer.cpp
    models/flight_control/FGActuator.cpp
    models/flight_control/FGAngles.cpp
    models/flight_control/FGDeadBand.cpp
    models/flight_control/FGFCSComponent.cpp
    models/flight_control/FGFCSFunction.cpp
    models/flight_control/FGFilter.cpp
    models/flight_control/FGGain.cpp
    models/flight_control/FGGyro.cpp
    models/flight_control/FGKinemat.cpp
    models/flight_control/FGMagnetometer.cpp
    models/flight_control/FGPID.cpp
    models/flight_control/FGSensor.cpp
    models/flight_control/FGSummer.cpp
    models/flight_control/FGSwitch.cpp
    models/flight_control/FGWaypoint.cpp
    models/flight_control/FGDistributor.cpp
    models/flight_control/FGLinearActuator.cpp
    models/propulsion/FGElectric.cpp
    models/propulsion/FGEngine.cpp
    models/propulsion/FGForce.cpp
    models/propulsion/FGNozzle.cpp
    models/propulsion/FGPiston.cpp
    models/propulsion/FGPropeller.cpp
    models/propulsion/FGRocket.cpp
    models/propulsion/FGRotor.cpp
    models/propulsion/FGTank.cpp
    models/propulsion/FGThruster.cpp
    models/propulsion/FGTransmission.cpp
    models/propulsion/FGTurbine.cpp
    models/propulsion/FGTurboProp.cpp
    )


add_library(JSBSim STATIC ${SOURCES} ${HEADERS})
set(VERSION_MESSAGE "compiled from FlightGear ${FLIGHTGEAR_VERSION}")
add_definitions("-DJSBSIM_VERSION=\"${VERSION_MESSAGE}\"")

target_link_libraries(JSBSim SimGearCore)
target_include_directories(JSBSim PRIVATE ${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)

add_executable(JSBsim_bin JSBSim.cpp )
set_target_properties(JSBsim_bin PROPERTIES OUTPUT_NAME "JSBSim" )
target_Link_libraries(JSBsim_bin JSBSim)
target_include_directories(JSBsim_bin PRIVATE ${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)

if (MSVC)
    set_target_properties(JSBsim_bin PROPERTIES DEBUG_POSTFIX d)
endif ()
install(TARGETS JSBsim_bin RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# eof
