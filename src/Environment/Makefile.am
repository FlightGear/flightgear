# libdir	= ${exec_prefix}/lib
# lib_LTLIBRARIES = libEnvironment.la
# libEnvironment_la_SOURCES = environment.c environment.h

noinst_LIBRARIES = libEnvironment.a

libEnvironment_a_SOURCES = \
	environment.cxx environment.hxx \
	environment_mgr.cxx environment_mgr.hxx \
	environment_ctrl.cxx environment_ctrl.hxx \
	metar.cxx metar.hxx

bin_PROGRAMS = metar

metar_SOURCES = metar-main.cxx metar.cxx metar.hxx

metar_LDADD = \
	-lsgio -lsgbucket -lsgmisc -lsgstructure -lsgdebug \
	-lplibnet -lplibul \
	-lz $(base_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
