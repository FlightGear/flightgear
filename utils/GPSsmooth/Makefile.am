noinst_PROGRAMS = GPSsmooth MIDGsmooth

GPSsmooth_SOURCES = \
	gps.cxx gps.hxx \
	gps_main.cxx

GPSsmooth_LDADD = \
	-lsgtiming -lsgmisc -lsgdebug -lplibnet -lplibul \
	$(joystick_LIBS) $(network_LIBS) $(base_LIBS) -lz

MIDGsmooth_SOURCES = \
	MIDG-II.cxx MIDG-II.hxx \
	MIDG_main.cxx

MIDGsmooth_LDADD = \
	-lsgio -lsgserial -lsgtiming -lsgmath -lsgbucket -lsgmisc -lsgdebug \
	-lplibnet -lplibul \
	$(joystick_LIBS) $(network_LIBS) $(base_LIBS) -lz

INCLUDES = -I$(top_srcdir)/src
