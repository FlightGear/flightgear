if ENABLE_UNIX_SERIAL
SERIAL_LIBS = $(top_builddir)/Lib/Serial/libSerial.a
else
SERIAL_LIBS =
endif

if ENABLE_OLD_WEATHER
WEATHER_LIBS =	$(top_builddir)/Simulator/Weather/libWeather.a
else
WEATHER_LIBS =	$(top_builddir)/Simulator/WeatherCM/libWeatherCM.a \
		$(top_builddir)/Lib/Voronoi/libVoronoi.a
endif


AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkglibdir)\"

EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in

bin_PROGRAMS = fgfs

bin_SCRIPTS = runfgfs runfgfs.bat

fgfs_SOURCES = \
	fg_init.cxx fg_init.hxx \
	fg_serial.cxx fg_serial.hxx \
	keyboard.cxx keyboard.hxx \
	main.cxx \
	options.cxx options.hxx \
	splash.cxx splash.hxx \
	views.cxx views.hxx

fgfs_LDADD = \
	$(top_builddir)/Simulator/Aircraft/libAircraft.a \
	$(top_builddir)/Simulator/Astro/libAstro.a \
	$(top_builddir)/Simulator/Autopilot/libAutopilot.a \
	$(top_builddir)/Simulator/Cockpit/libCockpit.a \
	$(top_builddir)/Simulator/Controls/libControls.a \
	$(top_builddir)/Simulator/FDM/libFlight.a \
	$(top_builddir)/Simulator/FDM/Balloon/libBalloon.a \
	$(top_builddir)/Simulator/FDM/External/libExternal.a \
	$(top_builddir)/Simulator/FDM/JSBsim/libJSBsim.a \
	$(top_builddir)/Simulator/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/Simulator/GUI/libGUI.a \
	$(top_builddir)/Simulator/Scenery/libScenery.a \
	$(top_builddir)/Simulator/Airports/libAirports.a \
        $(top_builddir)/Simulator/Network/libNetwork.a \
	$(top_builddir)/Simulator/Objects/libObjects.a \
	$(top_builddir)/Simulator/Clouds/libClouds.a \
	$(top_builddir)/Simulator/Time/libTime.a \
	$(WEATHER_LIBS) \
	$(top_builddir)/Simulator/Joystick/libJoystick.a \
	$(SERIAL_LIBS) \
	$(top_builddir)/Lib/Screen/libScreen.a \
	$(top_builddir)/Lib/Math/libMath.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Voronoi/libVoronoi.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	-lpu -lfnt -lssg -lsg \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a \
	$(opengl_LIBS) \
	$(audio_LIBS)


INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Simulator
