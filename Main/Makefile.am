
bindir = ${exec_prefix}/japhar/bin

bin_PROGRAMS	= fg
fg_SOURCES	= \
	fg_debug.c \
	fg_getopt.c \
	fg_init.c \
	views.c \
	GLUTkey.c \
	GLUTmain.c

# Need $(INTERFACE_LIBS) $(GRAPHICS_LIBS)
fg_LDADD	= \
	$(top_builddir)/Src/Aircraft/libAircraft.la \
	$(top_builddir)/Src/Astro/libAstro.la \
	$(top_builddir)/Src/Cockpit/libCockpit.la \
	$(top_builddir)/Src/Controls/libControls.la \
	$(top_builddir)/Src/Flight/libFlight.la \
	$(top_builddir)/Src/Flight/LaRCsim/libLaRCsim.la \
	$(top_builddir)/Src/Flight/Slew/libSlew.la \
	$(top_builddir)/Src/Scenery/libScenery.la \
	$(top_builddir)/Src/Time/libTime.la \
	$(top_builddir)/Src/Weather/libWeather.la \
        $(top_builddir)/Src/Joystick/libJoystick.la \
	$(top_builddir)/Src/Math/libMath.la

INCLUDES += -I.. -DGLUT
