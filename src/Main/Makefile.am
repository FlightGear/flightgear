if ENABLE_UNIX_SERIAL
SERIAL_LIBS = -lsgserial
else
SERIAL_LIBS =
endif

if ENABLE_OLD_WEATHER
WEATHER_LIBS = $(top_builddir)/src/Weather/libWeather.a
else
WEATHER_LIBS = $(top_builddir)/src/WeatherCM/libWeatherCM.a
endif

if ENABLE_NETWORK_OLK
NETWORK_LIBS = \
	$(top_builddir)/src/Network/libNetwork.a \
	$(top_builddir)/src/NetworkOLK/libNetworkOLK.a
else
NETWORK_LIBS = \
	$(top_builddir)/src/Network/libNetwork.a
endif

if OLD_AUTOMAKE
CXXFLAGS += -DPKGLIBDIR=\"$(pkglibdir)\"
else
AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkglibdir)\"
endif

EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in

bin_PROGRAMS = fgfs

bin_SCRIPTS = runfgfs runfgfs.bat

fgfs_SOURCES = \
	main.cxx \
	bfi.cxx bfi.hxx \
	fg_init.cxx fg_init.hxx \
	fg_io.cxx fg_io.hxx \
	globals.cxx globals.hxx \
	keyboard.cxx keyboard.hxx \
	options.cxx options.hxx \
	save.cxx save.hxx \
	splash.cxx splash.hxx \
	views.cxx views.hxx

fgfs_LDADD = \
	$(top_builddir)/src/Aircraft/libAircraft.a \
	$(top_builddir)/src/Autopilot/libAutopilot.a \
	$(top_builddir)/src/Cockpit/libCockpit.a \
	$(top_builddir)/src/Controls/libControls.a \
	$(top_builddir)/src/FDM/libFlight.a \
	$(top_builddir)/src/FDM/Balloon/libBalloon.a \
	$(top_builddir)/src/FDM/JSBSim/libJSBSim.a \
	$(top_builddir)/src/FDM/JSBSim/filtersjb/libfiltersjb.a \
	$(top_builddir)/src/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/src/FDM/UIUCModel/libUIUCModel.a \
	$(top_builddir)/src/GUI/libGUI.a \
	$(top_builddir)/src/Navaids/libNavaids.a \
	$(top_builddir)/src/Scenery/libScenery.a \
	$(top_builddir)/src/Airports/libAirports.a \
        $(NETWORK_LIBS) \
	$(top_builddir)/src/Objects/libObjects.a \
	$(top_builddir)/src/Time/libTime.a \
	$(WEATHER_LIBS) \
	$(top_builddir)/src/Joystick/libJoystick.a \
	-lsgsky -lsgephem -lsgtiming -lsgio -lsgscreen -lsgmath -lsgbucket \
	-lsgdebug -lsgmagvar -lsgmisc -lsgxml \
	$(SERIAL_LIBS) \
	-lplibpu -lplibfnt -lplibssg -lplibsg \
	-lmk4 -lz \
	$(opengl_LIBS) \
	$(audio_LIBS)

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
