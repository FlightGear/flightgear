SUBDIRS	= Balloon External JSBsim LaRCsim Slew

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	Balloon.cxx Balloon.hxx \
	flight.cxx flight.hxx \
	JSBsim.cxx JSBsim.hxx \
	LaRCsim.cxx LaRCsim.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator
