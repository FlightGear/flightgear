EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in \
             fg_os_sdl.cxx fg_os.cxx fg_os.hxx

if ENABLE_MPLAYER_AS
MPLAYER_LIBS = $(top_builddir)/src/MultiPlayer/libMultiPlayer.a
else
MPLAYER_LIBS = 
endif

if ENABLE_SP_FDM
SP_FDM_LIBS = $(top_builddir)/src/FDM/SP/libSPFDM.a
else
SP_FDM_LIBS = 
endif

if WITH_THREADS
THREAD_LIBS = -lsgthreads $(thread_LIBS)
else
THREAD_LIBS =
endif

if USE_SDL
GFX_CODE = fg_os_sdl.cxx fg_os.hxx
else
GFX_CODE = fg_os.cxx fg_os.hxx
endif

if FG_USE_CLOUDS_3D
CLOUD3D_LIBS = -lsgclouds3d
else
CLOUD3D_LIBS = 
endif

AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgfs metar

noinst_SCRIPTS = runfgfs.bat runfgfs

noinst_LIBRARIES = libMain.a

# bin_SCRIPTS = runfgfs

libMain_a_SOURCES = \
	main.cxx main.hxx \
	renderer.cxx renderer.hxx \
	fg_commands.cxx fg_commands.hxx \
	fg_init.cxx fg_init.hxx \
	fg_io.cxx fg_io.hxx \
	fg_props.cxx fg_props.hxx \
	globals.cxx globals.hxx \
        logger.cxx logger.hxx \
	options.cxx options.hxx \
	splash.cxx splash.hxx \
	util.cxx util.hxx \
	viewer.cxx viewer.hxx \
	viewmgr.cxx viewmgr.hxx \
	$(GFX_CODE)

fgfs_SOURCES = bootstrap.cxx

fgfs_LDADD = \
	$(top_builddir)/src/Main/libMain.a \
	$(top_builddir)/src/Aircraft/libAircraft.a \
	$(top_builddir)/src/ATC/libATC.a \
	$(top_builddir)/src/Cockpit/libCockpit.a \
	$(top_builddir)/src/Cockpit/built_in/libBuilt_in.a \
	$(top_builddir)/src/Controls/libControls.a \
	$(top_builddir)/src/FDM/libFlight.a \
	$(top_builddir)/src/FDM/Balloon/libBalloon.a \
	$(top_builddir)/src/FDM/ExternalNet/libExternalNet.a \
	$(top_builddir)/src/FDM/ExternalPipe/libExternalPipe.a \
	$(top_builddir)/src/FDM/JSBSim/libJSBSim.a \
	$(top_builddir)/src/FDM/YASim/libYASim.a \
	$(top_builddir)/src/FDM/JSBSim/filtersjb/libfiltersjb.a \
	$(top_builddir)/src/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/src/FDM/UIUCModel/libUIUCModel.a \
	$(SP_FDM_LIBS) \
	$(top_builddir)/src/GUI/libGUI.a \
	$(top_builddir)/src/Autopilot/libAutopilot.a \
	$(top_builddir)/src/Input/libInput.a \
	$(top_builddir)/src/Instrumentation/libInstrumentation.a \
	$(top_builddir)/src/Model/libModel.a \
	$(top_builddir)/src/AIModel/libAIModel.a \
	$(top_builddir)/src/Network/libNetwork.a \
	$(top_builddir)/src/Navaids/libNavaids.a \
	$(top_builddir)/src/Scenery/libScenery.a \
	$(top_builddir)/src/Scripting/libScripting.a \
	$(top_builddir)/src/Sound/libSound.a \
	$(top_builddir)/src/Airports/libAirports.a \
	$(MPLAYER_LIBS) \
	$(top_builddir)/src/Replay/libReplay.a \
	$(top_builddir)/src/Systems/libSystems.a \
	$(top_builddir)/src/Time/libTime.a \
	$(top_builddir)/src/Traffic/libTraffic.a \
	$(top_builddir)/src/Environment/libEnvironment.a \
	$(CLOUD3D_LIBS) \
	-lsgroute -lsgsky -lsgsound -lsgephem -lsgmaterial -lsgtgdb -lsgmodel \
	-lsgtiming -lsgio -lsgscreen -lsgmath -lsgbucket -lsgprops -lsgdebug \
	-lsgmagvar -lsgmisc -lsgnasal -lsgxml -lsgsound -lsgserial \
	-lsgstructure -lsgenvironment \
	$(THREAD_LIBS) \
	-lplibpu -lplibfnt -lplibjs -lplibnet -lplibssg -lplibsg -lplibul \
	$(network_LIBS) \
	-lz \
	$(opengl_LIBS) \
	$(openal_LIBS)

metar_SOURCES = metar_main.cxx

metar_LDADD = \
        -lsgenvironment -lsgio -lsgbucket -lsgmisc -lsgstructure -lsgdebug \
        -lplibnet -lplibul $(network_LIBS) \
        -lz $(base_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
