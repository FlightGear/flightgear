noinst_LIBRARIES = libJSBsim.a

libJSBsim_a_SOURCES = FGAircraft.cpp FGAircraft.h \
	FGAtmosphere.cpp FGAtmosphere.h \
	FGAuxiliary.cpp FGAuxiliary.h \
	FGCoefficient.cpp FGCoefficient.h \
	FGDefs.h \
	FGFCS.cpp FGFCS.h \
	FGFDMExec.cpp FGFDMExec.h \
	FGModel.cpp FGModel.h \
	FGOutput.cpp FGOutput.h \
	FGPosition.cpp FGPosition.h \
	FGRotation.cpp FGRotation.h \
	FGState.cpp FGState.h \
	FGTranslation.cpp FGTranslation.h \
	FGUtility.cpp FGUtility.h \
	FGEngine.cpp FGEngine.h \
	FGTank.cpp FGTank.h 

noinst_PROGRAMS = testJSBsim

testJSBsim_SOURCES = FGMain.cpp

testJSBsim_LDADD = libJSBsim.a

INCLUDES += -I$(top_builddir)

DEFS += -DFGFS
