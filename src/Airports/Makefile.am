noinst_LIBRARIES = libAirports.a

noinst_PROGRAMS = gensimple genrunways

libAirports_a_SOURCES = \
	runways.cxx runways.hxx \
	simple.cxx simple.hxx

gensimple_SOURCES = gensimple.cxx
gensimple_LDADD = libAirports.a -lsgdebug -lsgmisc -lmk4 -lz

genrunways_SOURCES = genrunways.cxx
genrunways_LDADD = libAirports.a -lsgdebug -lsgmisc -lmk4 -lz

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
