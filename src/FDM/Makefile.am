SUBDIRS	= Balloon JSBSim LaRCsim UIUCModel

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	Balloon.cxx Balloon.h \
	External.cxx External.hxx \
	flight.cxx flight.hxx \
	JSBSim.cxx JSBSim.hxx \
	LaRCsim.cxx LaRCsim.hxx \
	MagicCarpet.cxx MagicCarpet.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
