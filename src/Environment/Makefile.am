# libdir	= ${exec_prefix}/lib
# lib_LTLIBRARIES = libEnvironment.la
# libEnvironment_la_SOURCES = environment.c environment.h

noinst_LIBRARIES = libEnvironment.a

libEnvironment_a_SOURCES = \
	environment.cxx environment.hxx \
	environment_mgr.cxx environment_mgr.hxx \
	environment_ctrl.cxx environment_ctrl.hxx \
	fgmetar.cxx fgmetar.hxx fgclouds.cxx fgclouds.hxx \
	fgwind.cxx fgwind.hxx \
	atmosphere.cxx atmosphere.hxx \
	precipitation_mgr.cxx precipitation_mgr.hxx \
	ridge_lift.cxx ridge_lift.hxx \
	ephemeris.cxx ephemeris.hxx 

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
