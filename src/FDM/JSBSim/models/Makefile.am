SUBDIRS = atmosphere propulsion flight_control

noinst_LIBRARIES = libModels.a

libModels_a_SOURCES = FGAerodynamics.cpp FGAircraft.cpp FGAtmosphere.cpp \
                      FGAuxiliary.cpp FGFCS.cpp FGGroundReactions.cpp \
                      FGInertial.cpp FGBuoyantForces.cpp FGExternalForce.cpp \
                      FGLGear.cpp FGMassBalance.cpp FGModel.cpp FGOutput.cpp \
                      FGPropagate.cpp FGPropulsion.cpp FGInput.cpp \
                      FGExternalReactions.cpp FGGasCell.cpp FGAccelerations.cpp

noinst_HEADERS = FGAerodynamics.h FGAircraft.h FGAtmosphere.h FGAuxiliary.h \
                 FGFCS.h FGGroundReactions.h FGInertial.h FGLGear.h \
                 FGMassBalance.h FGBuoyantForces.h FGExternalForce.h \
                 FGModel.h FGOutput.h FGPropagate.h FGPropulsion.h FGInput.h \
                 FGExternalReactions.h FGGasCell.h FGAccelerations.h

INCLUDES = -I$(top_srcdir)/src/FDM/JSBSim
