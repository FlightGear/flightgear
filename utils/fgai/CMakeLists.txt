add_executable(fgai
  fgai.cxx
  HLAAircraft.cxx
  HLAAircraftClass.cxx
  HLAAirVehicle.cxx
  HLAAirVehicleClass.cxx
  AIBVHPager.cxx
  AIEnvironment.cxx
  AIManager.cxx
  AIObject.cxx
  AIPhysics.cxx
  HLABaloon.cxx
  HLABaloonClass.cxx
  HLAMPAircraft.cxx
  HLAMPAircraftClass.cxx
  HLASceneObject.cxx
  HLASceneObjectClass.cxx
)

target_link_libraries(fgai
  SimGearCore SimGearScene
  ${OPENSCENEGRAPH_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${RTI_LDFLAGS}
)

install(TARGETS fgai RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
