noinst_LIBRARIES = libAirports.a

noinst_PROGRAMS = gensimple genrunways calc_loc

libAirports_a_SOURCES = \
	runways.cxx runways.hxx \
	simple.cxx simple.hxx

gensimple_SOURCES = gensimple.cxx
gensimple_LDADD = libAirports.a -lsgdebug -lsgmisc -lsgxml -lmk4 -lz

genrunways_SOURCES = genrunways.cxx
genrunways_LDADD = libAirports.a -lsgdebug -lsgmisc -lsgxml -lmk4 -lz

calc_loc_SOURCES = calc_loc.cxx
calc_loc_LDADD = -lsgmath

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
