noinst_PROGRAMS = GPSsmooth

GPSsmooth_SOURCES = \
	gps.cxx gps.hxx \
	main.cxx

GPSsmooth_LDADD = \
	-lsgtiming -lsgmisc -lsgdebug -lplibnet -lplibul \
	$(joystick_LIBS) $(base_LIBS) -lz

INCLUDES = -I$(top_srcdir)/src
