SUBDIRS	= Balloon JSBSim LaRCsim UIUCModel YASim

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	ADA.cxx ADA.hxx \
	Balloon.cxx Balloon.h \
	External.cxx External.hxx \
	ExternalNet.cxx ExternalNet.hxx \
	flight.cxx flight.hxx \
	IO360.cxx IO360.hxx \
	JSBSim.cxx JSBSim.hxx \
	LaRCsim.cxx LaRCsim.hxx \
	LaRCsimIC.cxx LaRCsimIC.hxx \
	MagicCarpet.cxx MagicCarpet.hxx \
	NullFDM.cxx NullFDM.hxx

bin_PROGRAMS = pstest

pstest_SOURCES = ps-10520c.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
