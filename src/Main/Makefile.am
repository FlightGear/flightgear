if ENABLE_WEATHERCM
WEATHER_LIBS = $(top_builddir)/src/WeatherCM/libWeatherCM.a
else
WEATHER_LIBS = $(top_builddir)/src/Environment/libEnvironment.a
endif

if ENABLE_NETWORK_OLK
NETWORK_LIBS = \
	$(top_builddir)/src/Network/libNetwork.a \
	$(top_builddir)/src/NetworkOLK/libNetworkOLK.a
else
NETWORK_LIBS = \
	$(top_builddir)/src/Network/libNetwork.a
endif

if WITH_THREADS
THREAD_LIBS = -lsgthreads
else
THREAD_LIBS =
endif

if HAVE_PLIB_PSL
PSL_LIBS = -lplibpsl
else
PSL_LIBS =
endif

AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkglibdir)\"

EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in

bin_PROGRAMS = fgfs

noinst_SCRIPTS = runfgfs.bat runfgfs

# bin_SCRIPTS = runfgfs

fgfs_SOURCES = \
	main.cxx \
	fg_commands.cxx fg_commands.hxx \
	fg_init.cxx fg_init.hxx \
	fg_io.cxx fg_io.hxx \
	fg_props.cxx fg_props.hxx \
        fgfs.cxx fgfs.hxx \
	globals.cxx globals.hxx \
        logger.cxx logger.hxx \
	options.cxx options.hxx \
	splash.cxx splash.hxx \
	util.cxx util.hxx \
	viewer.cxx viewer.hxx \
	viewmgr.cxx viewmgr.hxx \
        location.cxx location.hxx

fgfs_LDADD = \
	$(top_builddir)/src/Aircraft/libAircraft.a \
	$(top_builddir)/src/ATC/libATC.a \
	$(top_builddir)/src/Autopilot/libAutopilot.a \
	$(top_builddir)/src/Cockpit/libCockpit.a \
	$(top_builddir)/src/Cockpit/built_in/libBuilt_in.a \
	$(top_builddir)/src/Controls/libControls.a \
	$(top_builddir)/src/FDM/libFlight.a \
	$(top_builddir)/src/FDM/Balloon/libBalloon.a \
	$(top_builddir)/src/FDM/ExternalNet/libExternalNet.a \
	$(top_builddir)/src/FDM/JSBSim/libJSBSim.a \
	$(top_builddir)/src/FDM/YASim/libYASim.a \
	$(top_builddir)/src/FDM/JSBSim/filtersjb/libfiltersjb.a \
	$(top_builddir)/src/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/src/FDM/UIUCModel/libUIUCModel.a \
	$(top_builddir)/src/GUI/libGUI.a \
	$(top_builddir)/src/Input/libInput.a \
	$(top_builddir)/src/Instrumentation/libInstrumentation.a \
	$(top_builddir)/src/Model/libModel.a \
	$(top_builddir)/src/Navaids/libNavaids.a \
	$(top_builddir)/src/Scenery/libScenery.a \
	$(top_builddir)/src/Scripting/libScripting.a \
	$(top_builddir)/src/Sound/libSound.a \
	$(top_builddir)/src/Airports/libAirports.a \
        $(NETWORK_LIBS) \
	$(top_builddir)/src/Objects/libObjects.a \
	$(top_builddir)/src/Systems/libSystems.a \
	$(top_builddir)/src/Time/libTime.a \
	$(WEATHER_LIBS) \
	-lsgroute -lsgsky -lsgephem -lsgtiming -lsgio -lsgscreen \
	-lsgmath -lsgbucket -lsgdebug -lsgmagvar -lsgmisc -lsgxml \
	-lsgserial \
	$(THREAD_LIBS) \
	-lplibpu -lplibfnt -lplibjs -lplibnet -lplibssg -lplibsg -lplibul \
        $(PSL_LIBS) \
	-lmk4 -lz \
	$(opengl_LIBS) \
	$(audio_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
