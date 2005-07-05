noinst_PROGRAMS = GPSsmooth

GPSsmooth_SOURCES = \
	gps.cxx gps.hxx \
	main.cxx

GPSsmooth_LDADD = -lsgtiming -lsgmisc -lplibnet -lz $(joystick_LIBS)

INCLUDES = -I$(top_srcdir)/src
