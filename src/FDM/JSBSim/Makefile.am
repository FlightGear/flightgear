SUBDIRS = filtersjb

EXTRA_DIST = JSBSim.cpp Makefile.solo

noinst_LIBRARIES = libJSBSim.a

# FGAerodynamics.cpp FGAerodynamics.h \
# FGGroundReactions.cpp FGGroundReactions.h \
# FGInertial.cpp FGInertial.h \
# FGMassBalance.cpp FGMassBalance.h \
# FGPiston.cpp FGPiston.h \
# FGPropulsion.cpp FGPropulsion.h \
# FGRocket.cpp FGRocket.h \
# FGTurboJet.cpp FGTurboJet.h \
# FGTurboShaft.cpp FGTurboShaft.h \

libJSBSim_a_SOURCES = \
	FGAircraft.cpp FGAircraft.h \
	FGAtmosphere.cpp FGAtmosphere.h \
	FGAuxiliary.cpp FGAuxiliary.h \
	FGCoefficient.cpp FGCoefficient.h \
	FGConfigFile.cpp FGConfigFile.h \
	FGControls.cpp FGControls.h \
	FGDefs.h \
	FGFCS.cpp FGFCS.h \
	FGFDMExec.cpp FGFDMExec.h \
	FGForce.cpp FGForce.h \
	FGInitialCondition.cpp FGInitialCondition.h \
	FGLGear.cpp FGLGear.h \
	FGMatrix.cpp FGMatrix.h \
	FGModel.cpp FGModel.h \
	FGNozzle.cpp FGNozzle.h \
	FGOutput.cpp FGOutput.h \
	FGPosition.cpp FGPosition.h \
	FGPropeller.cpp FGPropeller.h \
	FGRotation.cpp FGRotation.h \
	FGRotor.cpp FGRotor.h \
	FGState.cpp FGState.h \
	FGThruster.cpp FGThruster.h \
	FGTranslation.cpp FGTranslation.h \
	FGTrim.cpp FGTrim.h \
	FGTrimAxis.cpp FGTrimAxis.h \
	FGUtility.cpp FGUtility.h \
	FGEngine.cpp FGEngine.h \
	FGTank.cpp FGTank.h \
	FGfdmSocket.cpp FGfdmSocket.h

noinst_PROGRAMS = testJSBsim

testJSBsim_SOURCES = JSBSim.cpp

testJSBsim_LDADD = libJSBSim.a filtersjb/libfiltersjb.a

INCLUDES += -I$(top_srcdir)/src

DEFS += -DFGFS
