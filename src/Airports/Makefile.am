noinst_LIBRARIES = libAirports.a

noinst_PROGRAMS = calc_loc

libAirports_a_SOURCES = \
	runways.cxx runways.hxx \
	simple.cxx simple.hxx

calc_loc_SOURCES = calc_loc.cxx
calc_loc_LDADD = -lsgmath -lsgdebug -lsgmisc -lz

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
