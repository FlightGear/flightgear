SUBDIRS	= Balloon JSBsim LaRCsim

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	Balloon.cxx Balloon.h \
	External.cxx External.hxx \
	flight.cxx flight.hxx \
	JSBsim.cxx JSBsim.hxx \
	LaRCsim.cxx LaRCsim.hxx \
	MagicCarpet.cxx MagicCarpet.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator
