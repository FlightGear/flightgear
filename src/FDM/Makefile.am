SUBDIRS	= Balloon JSBSim LaRCsim UIUCModel

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	ADA.cxx ADA.hxx \
	Balloon.cxx Balloon.h \
	External.cxx External.hxx \
	flight.cxx flight.hxx \
	IO360.cxx IO360.hxx \
	JSBSim.cxx JSBSim.hxx \
	LaRCsim.cxx LaRCsim.hxx \
	LaRCsimIC.cxx LaRCsimIC.hxx \
	MagicCarpet.cxx MagicCarpet.hxx \
	NullFDM.cxx NullFDM.hxx

bin_PROGRAMS = pstest

pstest_SOURCES = ps-10520c.cxx

INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
