if ENABLE_SP_FDM
SP_DIR = SP
else
SP_DIR =
endif

SUBDIRS	= Balloon JSBSim LaRCsim UIUCModel YASim \
          $(SP_DIR) ExternalNet ExternalPipe

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	Balloon.cxx Balloon.h \
	flight.cxx flight.hxx \
	groundcache.cxx groundcache.hxx \
	MagicCarpet.cxx MagicCarpet.hxx \
	UFO.cxx UFO.hxx \
	NullFDM.cxx NullFDM.hxx

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
