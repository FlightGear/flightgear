EXTRA_DIST = runfg.in runfg.bat.in

bin_PROGRAMS = fg

bin_SCRIPTS = runfg runfg.bat

fg_SOURCES = \
	GLUTkey.c GLUTkey.h \
	GLUTmain.c \
	fg_config.h \
	fg_debug.c fg_debug.h \
	fg_getopt.c fg_getopt.h \
	fg_init.c fg_init.h \
	views.c views.h

fg_LDADD = \
	$(top_builddir)/Simulator/Aircraft/libAircraft.la \
	$(top_builddir)/Simulator/Astro/libAstro.la \
	$(top_builddir)/Simulator/Cockpit/libCockpit.la \
	$(top_builddir)/Simulator/Controls/libControls.la \
	$(top_builddir)/Simulator/Flight/libFlight.la \
	$(top_builddir)/Simulator/Flight/LaRCsim/libLaRCsim.la \
	$(top_builddir)/Simulator/Flight/Slew/libSlew.la \
	$(top_builddir)/Simulator/Scenery/libScenery.la \
	$(top_builddir)/Simulator/Scenery/Bucket/libBucket.la \
	$(top_builddir)/Simulator/Time/libTime.la \
	$(top_builddir)/Simulator/Weather/libWeather.la \
        $(top_builddir)/Simulator/Joystick/libJoystick.la \
	$(top_builddir)/Simulator/Math/libMath.la

INCLUDES += -I.. -DGLUT
