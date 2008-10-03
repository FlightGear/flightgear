SUBDIRS = atmosphere propulsion flight_control

noinst_LIBRARIES = libModels.a

libModels_a_SOURCES = FGAerodynamics.cpp FGAircraft.cpp FGAtmosphere.cpp \
                      FGAuxiliary.cpp FGFCS.cpp FGGroundReactions.cpp \
                      FGInertial.cpp FGLGear.cpp FGMassBalance.cpp FGModel.cpp \
                      FGOutput.cpp FGPropagate.cpp FGPropulsion.cpp \
                      FGInput.cpp FGBuoyantForces.cpp FGExternalForce.cpp \
                      FGGasCell.cpp FGExternalReactions.cpp

noinst_HEADERS = FGAerodynamics.h FGAircraft.h FGAtmosphere.h FGAuxiliary.h \
                 FGFCS.h FGGroundReactions.h FGInertial.h FGLGear.h FGInput.h \
                 FGMassBalance.h FGModel.h FGOutput.h FGPropagate.h \
                 FGPropulsion.h FGBuoyantForces.h FGExternalForce.h FGGasCell.h\
                 FGExternalReactions.h

INCLUDES = -I$(top_srcdir)/src/FDM/JSBSim
