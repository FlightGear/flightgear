EXTRA_DIST = Makefile.solo

noinst_LIBRARIES = libfiltersjb.a

libfiltersjb_a_SOURCES = \
	FGDeadBand.cpp FGDeadBand.h \
	FGFCSComponent.cpp FGFCSComponent.h \
	FGFilter.cpp FGFilter.h \
	FGGain.cpp FGGain.h \
	FGGradient.cpp FGGradient.h \
	FGKinemat.cpp FGKinemat.h \
	FGSummer.cpp FGSummer.h \
	FGSwitch.cpp FGSwitch.h \
        FGCondition.cpp FGCondition.h

AM_CXXFLAGS = -DFGFS

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
