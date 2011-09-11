EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in \
             fg_os_osgviewer.cxx fg_os.hxx

MPLAYER_LIBS = $(top_builddir)/src/MultiPlayer/libMultiPlayer.a

if ENABLE_SP_FDM
SP_FDM_LIBS = $(top_builddir)/src/FDM/SP/libSPFDM.a
else
SP_FDM_LIBS =
endif

if WITH_EVENTINPUT
EVENT_LIBS = $(eventinput_LIBS)
else
EVENT_LIBS =
endif

if HAVE_FRAMEWORK_PLIB
fgfs_PLIB_FW = $(plib_FRAMEWORK)
else
fgfs_PLIB_LIBS = -lplibpuaux -lplibpu -lplibfnt -lplibjs  \
	-lplibsg -lplibul 
endif

if HAVE_FRAMEWORK_OSG
fgfs_OSG_FW = $(osg_FRAMEWORKS) $(openthreads_FRAMEWORK)
endif

if WITH_HLA
HLA_LDADD=$(HLA_LDFLAGS) $(top_builddir)/src/Network/HLA/libFGHLA.a $(HLA_LIBS)
endif

GFX_CODE = fg_os_osgviewer.cxx fg_os_common.cxx fg_os.hxx

JSBSIM_LIBS = \
	$(top_builddir)/src/FDM/JSBSim/libJSBSim.a \
        $(top_builddir)/src/FDM/JSBSim/initialization/libInit.a \
        $(top_builddir)/src/FDM/JSBSim/models/atmosphere/libAtmosphere.a \
        $(top_builddir)/src/FDM/JSBSim/models/libModels.a \
        $(top_builddir)/src/FDM/JSBSim/models/flight_control/libFlightControl.a\
        $(top_builddir)/src/FDM/JSBSim/models/propulsion/libPropulsion.a \
        $(top_builddir)/src/FDM/JSBSim/input_output/libInputOutput.a \
        $(top_builddir)/src/FDM/JSBSim/math/libMath.a

AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgfs metar

noinst_SCRIPTS = runfgfs.bat runfgfs

noinst_LIBRARIES = libMain.a

# bin_SCRIPTS = runfgfs

libMain_a_SOURCES = \
	main.cxx main.hxx \
	renderer.cxx renderer.hxx \
	fgviewer.cxx fgviewer.hxx \
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
	CameraGroup.cxx CameraGroup.hxx \
	FGEventHandler.cxx FGEventHandler.hxx \
	WindowSystemAdapter.hxx WindowSystemAdapter.cxx \
	WindowBuilder.hxx WindowBuilder.cxx \
	$(GFX_CODE)

fgfs_SOURCES = bootstrap.cxx

fgfs_LDADD = \
	libMain.a \
	$(top_builddir)/src/Aircraft/libAircraft.a \
	$(top_builddir)/src/ATCDCL/libATCDCL.a \
	$(top_builddir)/src/Cockpit/libCockpit.a \
	$(top_builddir)/src/Cockpit/built_in/libBuilt_in.a \
	$(top_builddir)/src/Network/libNetwork.a \
        $(top_builddir)/src/FDM/libFlight.a \
	$(top_builddir)/src/FDM/ExternalNet/libExternalNet.a \
	$(top_builddir)/src/FDM/ExternalPipe/libExternalPipe.a \
	$(JSBSIM_LIBS) \
	$(top_builddir)/src/FDM/YASim/libYASim.a \
	$(top_builddir)/src/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/src/FDM/UIUCModel/libUIUCModel.a \
	$(SP_FDM_LIBS) \
	$(top_builddir)/src/GUI/libGUI.a \
	$(top_builddir)/src/Autopilot/libAutopilot.a \
	$(top_builddir)/src/Input/libInput.a \
	$(top_builddir)/src/Instrumentation/KLN89/libKLN89.a \
	$(top_builddir)/src/Instrumentation/libInstrumentation.a \
	$(top_builddir)/src/Instrumentation/HUD/libHUD.a \
	$(top_builddir)/src/Model/libModel.a \
	$(top_builddir)/src/Navaids/libNavaids.a \
	$(top_builddir)/src/Scenery/libScenery.a \
	$(top_builddir)/src/Scripting/libScripting.a \
	$(top_builddir)/src/Sound/libSound.a \
	$(top_builddir)/src/Airports/libAirports.a \
	$(MPLAYER_LIBS) \
        $(top_builddir)/src/AIModel/libAIModel.a \
	$(top_builddir)/src/ATC/libATC.a \
	$(top_builddir)/src/Systems/libSystems.a \
	$(top_builddir)/src/Time/libTime.a \
	$(top_builddir)/src/Traffic/libTraffic.a \
	$(top_builddir)/src/Environment/libEnvironment.a \
	$(HLA_LDADD) \
	-lsgroute -lsgsky -lsgsound -lsgephem -lsgtgdb -lsgmodel -lsgbvh \
	-lsgmaterial -lsgutil -lsgtiming -lsgio -lsgscreen -lsgmath -lsgbucket \
	-lsgprops -lsgdebug -lsgmagvar -lsgmisc -lsgnasal -lsgxml -lsgsound \
	-lsgserial -lsgstructure -lsgenvironment -lsgtsync -lsgthreads \
        $(fgfs_PLIB_LIBS) \
	$(OSG_LIBS) \
	$(thread_LIBS) \
	$(network_LIBS) \
	-lz \
	$(opengl_LIBS) \
	$(openal_LIBS) \
	$(EVENT_LIBS) \
	$(svn_LIBS)

fgfs_LDFLAGS = $(fgfs_PLIB_FW) $(fgfs_OSG_FW) $(LDFLAGS)

metar_SOURCES = metar_main.cxx

metar_LDADD = \
        -lsgenvironment -lsgio -lsgthreads -lsgbucket -lsgmisc -lsgstructure \
        -lsgdebug $(network_LIBS) $(thread_LIBS) \
        -lz $(base_LIBS)


INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_builddir)/src $(HLA_CPPFLAGS)
