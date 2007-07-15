noinst_LIBRARIES = libAirports.a

noinst_PROGRAMS = calc_loc

libAirports_a_SOURCES = \
	apt_loader.cxx apt_loader.hxx \
	runways.cxx runways.hxx \
	simple.cxx simple.hxx \
	runwayprefs.cxx runwayprefs.hxx \
	parking.cxx parking.hxx \
	gnnode.cxx gnnode.hxx \
	groundnetwork.cxx groundnetwork.hxx \
	dynamics.cxx dynamics.hxx \
	trafficcontrol.cxx trafficcontrol.hxx \
	dynamicloader.cxx dynamicloader.hxx \
	runwayprefloader.cxx runwayprefloader.hxx \
	xmlloader.cxx xmlloader.hxx 

calc_loc_SOURCES = calc_loc.cxx
calc_loc_LDADD = -lsgmath -lsgdebug -lsgmisc -lz $(base_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
