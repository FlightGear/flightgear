if ENABLE_SP_FDM
SP_DIR = SP
else
SP_DIR =
endif

SUBDIRS	= JSBSim LaRCsim UIUCModel YASim \
          $(SP_DIR) ExternalNet ExternalPipe

noinst_LIBRARIES = libFlight.a

libFlight_a_SOURCES = \
	flight.cxx flight.hxx \
	groundcache.cxx groundcache.hxx \
	UFO.cxx UFO.hxx \
	NullFDM.cxx NullFDM.hxx

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
