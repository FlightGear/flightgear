EXTRA_DIST = Makefile.solo

noinst_LIBRARIES = libfiltersjb.a

libfiltersjb_a_SOURCES = \
	FGDeadBand.cpp FGDeadBand.h \
	FGFCSComponent.cpp FGFCSComponent.h \
	FGFilter.cpp FGFilter.h \
	FGFlaps.cpp FGFlaps.h \
	FGGain.cpp FGGain.h \
	FGGradient.cpp FGGradient.h \
	FGSummer.cpp FGSummer.h \
	FGSwitch.cpp FGSwitch.h

INCLUDES += -I$(top_srcdir)/src

DEFS += -DFGFS
