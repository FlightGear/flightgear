noinst_LIBRARIES = libAirports.a

noinst_PROGRAMS = buildsimple

libAirports_a_SOURCES = \
	simple.cxx simple.hxx

buildsimple_SOURCES = buildsimple.cxx

buildsimple_LDADD = libAirports.a -lsgdebug -lsgmisc -lmk4 -lz

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
