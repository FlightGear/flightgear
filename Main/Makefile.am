
# bindir = ${exec_prefix}/japhar/bin

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
	$(top_builddir)/Simulator/Aircraft/libAircraft.la \
	$(top_builddir)/Simulator/Astro/libAstro.la \
	$(top_builddir)/Simulator/Cockpit/libCockpit.la \
	$(top_builddir)/Simulator/Controls/libControls.la \
	$(top_builddir)/Simulator/Flight/libFlight.la \
	$(top_builddir)/Simulator/Flight/LaRCsim/libLaRCsim.la \
	$(top_builddir)/Simulator/Flight/Slew/libSlew.la \
	$(top_builddir)/Simulator/Scenery/libScenery.la \
	$(top_builddir)/Simulator/Time/libTime.la \
	$(top_builddir)/Simulator/Weather/libWeather.la \
        $(top_builddir)/Simulator/Joystick/libJoystick.la \
	$(top_builddir)/Simulator/Math/libMath.la

INCLUDES += -I.. -DGLUT
