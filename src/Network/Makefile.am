noinst_LIBRARIES = libNetwork.a

if ENABLE_JPEG_SERVER
JPEG_SERVER = jpg-httpd.cxx jpg-httpd.hxx
else
JPEG_SERVER =
endif

libNetwork_a_SOURCES = \
	protocol.cxx protocol.hxx \
	atlas.cxx atlas.hxx \
	garmin.cxx garmin.hxx \
        httpd.cxx httpd.hxx \
        $(JPEG_SERVER) \
	joyclient.cxx joyclient.hxx \
	native.cxx native.hxx \
	native_ctrls.cxx native_ctrls.hxx \
	native_fdm.cxx native_fdm.hxx \
	nmea.cxx nmea.hxx \
        opengc.cxx opengc.hxx opengc_data.hxx \
	props.cxx props.hxx \
	pve.cxx pve.hxx \
	raw_ctrls.hxx raw_fdm.hxx \
	ray.cxx ray.hxx \
	rul.cxx rul.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
