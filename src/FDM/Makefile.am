SUBDIRS	= Balloon JSBSim LaRCsim UIUCModel

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	10520d.cxx 10520d.hxx \
	Balloon.cxx Balloon.h \
	External.cxx External.hxx \
	flight.cxx flight.hxx \
	JSBSim.cxx JSBSim.hxx \
	LaRCsim.cxx LaRCsim.hxx \
	MagicCarpet.cxx MagicCarpet.hxx

bin_PROGRAMS = engine

engine_SOURCES = engine.cxx

engine_LDADD = libFlight.a

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
