EXTRA_DIST = jpg-httpd.cxx jpg-httpd.hxx \
             multiplay.cxx multiplay.hxx

noinst_LIBRARIES = libNetwork.a

if ENABLE_JPEG_SERVER
JPEG_SERVER = jpg-httpd.cxx jpg-httpd.hxx
else
JPEG_SERVER =
endif

MPLAYER_AS = multiplay.cxx multiplay.hxx

libNetwork_a_SOURCES = \
	protocol.cxx protocol.hxx \
	ATC-Main.cxx ATC-Main.hxx \
	ATC-Inputs.cxx ATC-Inputs.hxx \
	ATC-Outputs.cxx ATC-Outputs.hxx \
	atlas.cxx atlas.hxx \
	AV400.cxx AV400.hxx \
	AV400Sim.cxx AV400Sim.hxx \
	AV400WSim.cxx AV400WSim.hxx \
	garmin.cxx garmin.hxx \
	lfsglass.cxx lfsglass.hxx lfsglass_data.hxx \
        httpd.cxx httpd.hxx \
        $(JPEG_SERVER) \
	joyclient.cxx joyclient.hxx \
	jsclient.cxx jsclient.hxx \
	native.cxx native.hxx \
	native_ctrls.cxx native_ctrls.hxx \
	native_fdm.cxx native_fdm.hxx \
	native_gui.cxx native_gui.hxx \
	net_ctrls.hxx net_fdm.hxx net_fdm_mini.hxx net_gui.hxx \
        nmea.cxx nmea.hxx \
        opengc.cxx opengc.hxx opengc_data.hxx \
	$(MPLAYER_AS) \
	props.cxx props.hxx \
	pve.cxx pve.hxx \
	ray.cxx ray.hxx \
	rul.cxx rul.hxx \
	generic.hxx generic.cxx

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

if WITH_HLA
SUBDIRS = HLA
endif
