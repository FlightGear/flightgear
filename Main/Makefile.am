if ENABLE_AUDIO_SUPPORT
DEFS += -DENABLE_AUDIO_SUPPORT
AUDIO_LIBS = \
	$(top_builddir)/Lib/Audio/src/libsl.la \
	$(top_builddir)/Lib/Audio/src/libsm.la
endif

if ENABLE_WIN32_AUDIO
LIBS += -lwinmm
endif

if ENABLE_XMESA_FX
DEFS += -DXMESA -DFX
endif

EXTRA_DIST = runfg.in runfg.bat.in

bin_PROGRAMS = fg

bin_SCRIPTS = runfg runfg.bat

fg_SOURCES = \
	GLUTkey.cxx GLUTkey.hxx \
	GLUTmain.cxx \
	airports.cxx airports.hxx \
	fg_config.h \
	fg_init.cxx fg_init.hxx \
	options.cxx options.hxx \
	splash.cxx splash.hxx \
	views.cxx views.hxx

fg_LDADD = \
	$(top_builddir)/Simulator/Aircraft/libAircraft.a \
	$(top_builddir)/Simulator/Astro/libAstro.a \
	$(top_builddir)/Simulator/Autopilot/libAutopilot.a \
	$(top_builddir)/Simulator/Cockpit/libCockpit.a \
	$(top_builddir)/Simulator/Controls/libControls.a \
	$(top_builddir)/Simulator/Flight/libFlight.a \
	$(top_builddir)/Simulator/Flight/LaRCsim/libLaRCsim.a \
	$(top_builddir)/Simulator/Flight/Slew/libSlew.a \
	$(top_builddir)/Simulator/GUI/libGUI.a \
	$(top_builddir)/Simulator/Scenery/libScenery.a \
	$(top_builddir)/Simulator/Time/libTime.a \
	$(top_builddir)/Simulator/Weather/libWeather.a \
        $(top_builddir)/Simulator/Joystick/libJoystick.a \
	$(AUDIO_LIBS) \
	$(top_builddir)/Lib/Math/libMath.la \
	$(top_builddir)/Lib/Bucket/libBucket.la \
	$(top_builddir)/Lib/Debug/libDebug.la \
	$(top_builddir)/Lib/PUI/libPUI.la \
	$(top_builddir)/Lib/zlib/libz.la

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Simulator
