noinst_PROGRAMS = GPSsmooth MIDGsmooth UGsmooth

if HAVE_FRAMEWORK_PLIB
GPSsmooth_LDFLAGS = $(plib_FRAMEWORK)
MIDGsmooth_LDFLAGS = $(plib_FRAMEWORK)
UGsmooth_LDFLAGS = $(plib_FRAMEWORK)
else
GPSsmooth_PLIB_LIBS = -lplibnet -lplibul
MIDGsmooth_PLIB_LIBS = -lplibnet -lplibul
UGsmooth_PLIB_LIBS = -lplibnet -lplibul
endif


GPSsmooth_SOURCES = \
	gps.cxx gps.hxx \
	gps_main.cxx

GPSsmooth_LDADD = \
        $(GPSsmooth_PLIB_LIBS) -lsgio -lsgtiming -lsgmisc -lsgdebug -lsgstructure \
	$(joystick_LIBS) $(network_LIBS) $(base_LIBS) -lz

MIDGsmooth_SOURCES = \
	MIDG-II.cxx MIDG-II.hxx \
	MIDG_main.cxx

MIDGsmooth_LDADD = \
        $(MIDGsmooth_PLIB_LIBS) -lsgio -lsgserial -lsgtiming \
        -lsgmath -lsgbucket -lsgmisc -lsgdebug -lsgstructure \
	$(joystick_LIBS) $(network_LIBS) $(base_LIBS) -lz

UGsmooth_SOURCES = \
        UGear.cxx UGear.hxx \
	UGear_command.cxx UGear_command.hxx \
        UGear_main.cxx \
	UGear_opengc.hxx \
	UGear_telnet.cxx UGear_telnet.hxx

UGsmooth_LDADD = \
        $(UGsmooth_PLIB_LIBS) -lsgio -lsgserial -lsgtiming \
        -lsgmath -lsgbucket -lsgmisc -lsgdebug -lsgstructure \
        $(joystick_LIBS) $(network_LIBS) $(base_LIBS) -lz

INCLUDES = -I$(top_srcdir)/src
