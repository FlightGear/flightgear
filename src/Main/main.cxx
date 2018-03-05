// main.cxx -- top level sim routines
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props_io.hxx>

#include <iostream>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osgDB/Registry>

#if defined(HAVE_CRASHRPT)
	#include <CrashRpt.h>

// defined in bootstrap.cxx
extern bool global_crashRptEnabled;

#endif

// Class references
#include <simgear/canvas/VGInitOperation.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/scene/tsync/terrasync.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/strutils.hxx>

#include <Add-ons/AddonManager.hxx>
#include <Main/locale.hxx>
#include <Model/panelnode.hxx>
#include <Scenery/scenery.hxx>
#include <Sound/soundmanager.hxx>
#include <Time/TimeManager.hxx>
#include <GUI/gui.h>
#include <GUI/MessageBox.hxx>
#include <Viewer/splash.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Include/version.h>

#include "fg_commands.hxx"
#include "fg_io.hxx"
#include "main.hxx"
#include "util.hxx"
#include "fg_init.hxx"
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "positioninit.hxx"
#include "screensaver_control.hxx"
#include "subsystemFactory.hxx"
#include "options.hxx"

#include <simgear/embedded_resources/EmbeddedResourceManager.hxx>
#include <EmbeddedResources/FlightGear-resources.hxx>

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

using namespace flightgear;

using std::cerr;
using std::vector;

// The atexit() function handler should know when the graphical subsystem
// is initialized.
extern int _bootstrap_OSInit;

static SGPropertyNode_ptr frame_signal;

// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void )
{

    if (sglog().has_popup()) {
        std::string s = sglog().get_popup();
        flightgear::modalMessageBox("Alert", s, "");
    }

    frame_signal->fireValueChanged();

    auto timeManager = globals->get_subsystem<TimeManager>();
    // compute simulated time (allowing for pause, warp, etc) and
    // real elapsed time
    double sim_dt, real_dt;
    timeManager->computeTimeDeltas(sim_dt, real_dt);

    // update all subsystems
    globals->get_subsystem_mgr()->update(sim_dt);

    simgear::AtomicChangeListener::fireChangeListeners();
}

static void initTerrasync()
{
    // add the terrasync root as a data path so data can be retrieved from it
    // (even if we are in read-only mode)
    SGPath terraSyncDir(globals->get_terrasync_dir());
    globals->append_data_path(terraSyncDir);

    if (fgGetBool("/sim/fghome-readonly", false)) {
        return;
    }

    // start TerraSync up now, so it can be synchronizing shared models
    // and airports data in parallel with a nav-cache rebuild.
    SGPath tsyncCache(terraSyncDir);
    tsyncCache.append("terrasync-cache.xml");

    // wipe the cache file if requested
    if (flightgear::Options::sharedInstance()->isOptionSet("restore-defaults")) {
        SG_LOG(SG_GENERAL, SG_INFO, "restore-defaults requested, wiping terrasync update cache at " <<
               tsyncCache);
        if (tsyncCache.exists()) {
            tsyncCache.remove();
        }
    }

    fgSetString("/sim/terrasync/cache-path", tsyncCache.utf8Str());

    // make fg-root dir available so existing Scenery data can be copied, and
    // hence not downloaded again.
    fgSetString("/sim/terrasync/installation-dir", (globals->get_fg_root() / "Scenery").utf8Str());

    simgear::SGTerraSync* terra_sync = new simgear::SGTerraSync();
    terra_sync->setRoot(globals->get_props());
    globals->add_subsystem("terrasync", terra_sync);

    terra_sync->bind();
    terra_sync->init();
}

static void fgSetVideoOptions()
{
    std::string vendor = fgGetString("/sim/rendering/gl-vendor");
    SGPath path(globals->get_fg_root());
    path.append("Video");
    path.append(vendor);
    if (path.exists())
    {
        std::string renderer = fgGetString("/sim/rendering/gl-renderer");
        size_t pos = renderer.find("x86/");
        if (pos == std::string::npos) {
            pos = renderer.find('/');
        }
        if (pos == std::string::npos) {
            pos = renderer.find(" (");
        }
        if (pos != std::string::npos) {
            renderer = renderer.substr(0, pos);
        }
        path.append(renderer+".xml");
        if (path.exists()) {
            SG_LOG(SG_INPUT, SG_INFO, "Reading video settings from " << path);
            try {
                SGPropertyNode *r_prop = fgGetNode("/sim/rendering");
                readProperties(path, r_prop);
            } catch (sg_exception& e) {
                SG_LOG(SG_INPUT, SG_WARN, "failed to read video settings:" << e.getMessage()
                << "(from " << e.getOrigin() << ")");
            }

            flightgear::Options* options = flightgear::Options::sharedInstance();
            if (!options->isOptionSet("ignore-autosave")) {
                globals->loadUserSettings();
            }
        }
    }
}

static void checkOpenGLVersion()
{
#if defined(SG_MAC)
    // Mac users can't upgrade their drivers, so complaining about
    // versions doesn't help them much
    return;
#endif

    // format of these strings is not standardised, so be careful about
    // parsing them.
    std::string versionString(fgGetString("/sim/rendering/gl-version"));
    string_list parts = simgear::strutils::split(versionString);
    if (parts.size() == 3) {
        if (parts[1].find("NVIDIA") != std::string::npos) {
            // driver version number, dot-seperared
            string_list driverVersion = simgear::strutils::split(parts[2], ".");
            if (!driverVersion.empty()) {
                int majorDriverVersion = simgear::strutils::to_int(driverVersion[0]);
                if (majorDriverVersion < 300) {
                    std::ostringstream ss;
                    ss << "Please upgrade to at least version 300 of the nVidia drivers (installed version is " << parts[2] << ")";

                    flightgear::modalMessageBox("Outdated graphics drivers",
                                                "FlightGear has detected outdated drivers for your graphics card.",
                                                ss.str());
                }
            }
        } // of NVIDIA-style version string
    } // of three parts
}

static void registerMainLoop()
{
    // stash current frame signal property
    frame_signal = fgGetNode("/sim/signals/frame", true);
    fgRegisterIdleHandler( fgMainLoop );
}

// This is the top level master main function that is registered as
// our idle function

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static int idle_state = 0;

static void fgIdleFunction ( void ) {
    // Specify our current idle function state.  This is used to run all
    // our initializations out of the idle callback so that we can get a
    // splash screen up and running right away.

    if ( idle_state == 0 ) {
        if (guiInit())
        {
            checkOpenGLVersion();
            fgSetVideoOptions();
            idle_state+=2;
            fgSplashProgress("loading-aircraft-list");
            fgSetBool("/sim/rendering/initialized", true);
        }

    } else if ( idle_state == 2 ) {
        initTerrasync();
        idle_state++;
        fgSplashProgress("loading-nav-dat");

    } else if ( idle_state == 3 ) {

        bool done = fgInitNav();
        if (done) {
          ++idle_state;
          fgSplashProgress("init-scenery");
        }
    } else if ( idle_state == 4 ) {
        idle_state++;

        globals->add_new_subsystem<TimeManager>(SGSubsystemMgr::INIT);

        // Do some quick general initializations
        if( !fgInitGeneral()) {
            throw sg_exception("General initialization failed");
        }

        ////////////////////////////////////////////////////////////////////
        // Initialize the property-based built-in commands
        ////////////////////////////////////////////////////////////////////
        fgInitCommands();
				fgInitSceneCommands();

        flightgear::registerSubsystemCommands(globals->get_commands());

        ////////////////////////////////////////////////////////////////////
        // Initialize the material manager
        ////////////////////////////////////////////////////////////////////
        globals->set_matlib( new SGMaterialLib );
        simgear::SGModelLib::setPanelFunc(FGPanelNode::load);

    } else if (( idle_state == 5 ) || (idle_state == 2005)) {
        idle_state+=2;
        flightgear::initPosition();
        flightgear::initTowerLocationListener();

        simgear::SGModelLib::init(globals->get_fg_root().local8BitStr(), globals->get_props());

        auto timeManager = globals->get_subsystem<TimeManager>();
        timeManager->init();

        ////////////////////////////////////////////////////////////////////
        // Initialize the TG scenery subsystem.
        ////////////////////////////////////////////////////////////////////

        globals->add_new_subsystem<FGScenery>(SGSubsystemMgr::DISPLAY);
        globals->get_scenery()->init();
        globals->get_scenery()->bind();

        fgSplashProgress("creating-subsystems");
    } else if (( idle_state == 7 ) || (idle_state == 2007)) {
        bool isReset = (idle_state == 2007);
        idle_state = 8; // from the next state on, reset & startup are identical
        SGTimeStamp st;
        st.stamp();
        fgCreateSubsystems(isReset);
        SG_LOG(SG_GENERAL, SG_INFO, "Creating subsystems took:" << st.elapsedMSec());
        fgSplashProgress("binding-subsystems");

    } else if ( idle_state == 8 ) {
        idle_state++;
        SGTimeStamp st;
        st.stamp();
        globals->get_subsystem_mgr()->bind();
        SG_LOG(SG_GENERAL, SG_INFO, "Binding subsystems took:" << st.elapsedMSec());

        fgSplashProgress("init-subsystems");
    } else if ( idle_state == 9 ) {
        SGSubsystem::InitStatus status = globals->get_subsystem_mgr()->incrementalInit();
        if ( status == SGSubsystem::INIT_DONE) {
          ++idle_state;
          fgSplashProgress("finishing-subsystems");
        } else {
          fgSplashProgress("init-subsystems");
        }

    } else if ( idle_state == 10 ) {
        idle_state = 900;
        fgPostInitSubsystems();
        fgSplashProgress("finalize-position");
    } else if ( idle_state == 900 ) {
        idle_state = 1000;

        // setup OpenGL view parameters
        globals->get_renderer()->setupView();

        globals->get_renderer()->resize( fgGetInt("/sim/startup/xsize"),
                                         fgGetInt("/sim/startup/ysize") );
        WindowSystemAdapter::getWSA()->windows[0]->gc->add(
          new simgear::canvas::VGInitOperation()
        );

        int session = fgGetInt("/sim/session",0);
        session++;
        fgSetInt("/sim/session",session);
    }

    if ( idle_state == 1000 ) {
        sglog().setStartupLoggingEnabled(false);

        // We've finished all our initialization steps, from now on we
        // run the main loop.
        fgSetBool("sim/sceneryloaded", false);
        registerMainLoop();
    }

    if ( idle_state == 2000 ) {
        fgStartNewReset();
        idle_state = 2005;
    }
}

void fgResetIdleState()
{
    idle_state = 2000;
    fgRegisterIdleHandler( &fgIdleFunction );
}

void fgInitSecureMode()
{
    bool secureMode = true;
    if (Options::sharedInstance()->isOptionSet("allow-nasal-from-sockets")) {
        SG_LOG(SG_GENERAL, SG_ALERT, "\n!! Network connections allowed to use Nasal !!\n"
       "Network connections will be allowed full access to the simulator \n"
       "including running arbitrary scripts. Ensure you have adequate security\n"
        "(such as a firewall which blocks external connections).\n");
        secureMode = false;
    }
    
    // it's by design that we overwrite any existing property tree value
    // here - this prevents an aircraft or add-on setting the property
    // value underneath us, eg in their -set.xml
    SGPropertyNode_ptr secureFlag = fgGetNode("/sim/secure-flag", true);
    secureFlag->setBoolValue(secureMode);
    secureFlag->setAttributes(SGPropertyNode::READ |
                              SGPropertyNode::PRESERVE |
                              SGPropertyNode::PROTECTED);
}

static void upper_case_property(const char *name)
{
    using namespace simgear;
    SGPropertyNode *p = fgGetNode(name, false);
    if (!p) {
        p = fgGetNode(name, true);
        p->setStringValue("");
    } else {
        props::Type t = p->getType();
        if (t == props::NONE || t == props::UNSPECIFIED)
            p->setStringValue("");
        else
            assert(t == props::STRING);
    }
    SGPropertyChangeListener* muc = new FGMakeUpperCase;
    globals->addListenerToCleanup(muc);
    p->addChangeListener(muc);
}

// this hack is needed to avoid weird viewport sizing within OSG on Windows.
// still required as of March 2017, sad times.
// see for example https://sourceforge.net/p/flightgear/codetickets/1958/
static void ATIScreenSizeHack()
{
    osg::ref_ptr<osg::Camera> hackCam = new osg::Camera;
    hackCam->setRenderOrder(osg::Camera::PRE_RENDER);
    int prettyMuchAnyInt = 1;
    hackCam->setViewport(0, 0, prettyMuchAnyInt, prettyMuchAnyInt);
    globals->get_renderer()->addCamera(hackCam, false);
}

// Propose NVIDIA Optimus / AMD Xpress to use high-end GPU
#if defined(SG_WINDOWS)
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void logToHome()
{
    SGPath logPath = globals->get_fg_home();
    logPath.append("fgfs.log");
    if (logPath.exists()) {
        SGPath prevLogPath = globals->get_fg_home();
        prevLogPath.append("fgfs_0.log");
        logPath.rename(prevLogPath);
        // bit strange, we need to restore the correct value of logPath now
        logPath = globals->get_fg_home();
        logPath.append("fgfs.log");
    }
    sglog().logToFile(logPath, SG_ALL, SG_INFO);

#if defined(HAVE_CRASHRPT)
	if (global_crashRptEnabled) {
		crAddFile2(logPath.c_str(), NULL, "FlightGear Log File", CR_AF_MAKE_FILE_COPY);
		SG_LOG( SG_GENERAL, SG_INFO, "CrashRpt enabled");
	} else {
		SG_LOG(SG_GENERAL, SG_WARN, "CrashRpt enabled at compile time but failed to install");
	}
#endif
}

// Main top level initialization
int fgMainInit( int argc, char **argv )
{
    // set default log level to 'info' for startup, we will revert to a lower
    // level once startup is done.
    sglog().setLogLevels( SG_ALL, SG_INFO );
    sglog().setStartupLoggingEnabled(true);

    globals = new FGGlobals;
    if (!fgInitHome()) {
        return EXIT_FAILURE;
    }

    const bool readOnlyFGHome = fgGetBool("/sim/fghome-readonly");
    if (!readOnlyFGHome) {
        // now home is initialised, we can log to a file inside it
        logToHome();
    }

    std::string version(FLIGHTGEAR_VERSION);
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Version " << version );
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Build Type " << FG_BUILD_TYPE );
    SG_LOG( SG_GENERAL, SG_INFO, "Built with " << SG_COMPILER_STR);
	SG_LOG( SG_GENERAL, SG_INFO, "Jenkins number/ID " << HUDSON_BUILD_NUMBER << ":"
			<< HUDSON_BUILD_ID);

    // seed the random number generator
    sg_srandom_time();

    string_list *col = new string_list;
    globals->set_channel_options_list( col );

    fgValidatePath(globals->get_fg_home(), false);  // initialize static variables
    upper_case_property("/sim/presets/airport-id");
    upper_case_property("/sim/presets/runway");
    upper_case_property("/sim/tower/airport-id");
    upper_case_property("/autopilot/route-manager/input");

// check if the launcher is requested, since it affects config file parsing
    bool showLauncher = flightgear::Options::checkForArg(argc, argv, "launcher");
    // an Info.plist bundle can't define command line arguments, but it can set
    // environment variables. This avoids needed a wrapper shell-script on OS-X.
    showLauncher |= (::getenv("FG_LAUNCHER") != 0);
    if (showLauncher) {
        // to minimise strange interactions when launcher and config files
        // set overlaping options, we disable the default files. Users can
        // still explicitly request config files via --config options if they choose.
        flightgear::Options::sharedInstance()->setShouldLoadDefaultConfig(false);
    }

    if (showLauncher && readOnlyFGHome) {
        // this is perhaps not what the user wanted, let's inform them
        flightgear::modalMessageBox("Multiple copies of FlightGear",
                                    "Another copy of FlightGear is already running on this computer, "
                                    "so this copy will run in read-only mode.");
    }

    // Load the configuration parameters.  (Command line options
    // override config file options.  Config file options override
    // defaults.)
    int configResult = fgInitConfig(argc, argv, false);
    if (configResult == flightgear::FG_OPTIONS_ERROR) {
        return EXIT_FAILURE;
    } else if (configResult == flightgear::FG_OPTIONS_EXIT) {
        return EXIT_SUCCESS;
    }

#if defined(HAVE_QT)
    if (showLauncher) {
        flightgear::initApp(argc, argv);
        flightgear::checkKeyboardModifiersForSettingFGRoot();

        if (!flightgear::runLauncherDialog()) {
            return EXIT_SUCCESS;
        }
    }
#else
    if (showLauncher) {
        SG_LOG(SG_GENERAL, SG_ALERT, "\n!Launcher requested, but FlightGear was compiled without Qt support!\n");
    }
#endif
    
    fgInitSecureMode();
    fgInitAircraftPaths(false);

    configResult = fgInitAircraft(false);
    if (configResult == flightgear::FG_OPTIONS_ERROR) {
        return EXIT_FAILURE;
    } else if (configResult == flightgear::FG_OPTIONS_EXIT) {
        return EXIT_SUCCESS;
    }

    addons::AddonManager::createInstance();

    configResult = flightgear::Options::sharedInstance()->processOptions();
    if (configResult == flightgear::FG_OPTIONS_ERROR) {
        return EXIT_FAILURE;
    } else if (configResult == flightgear::FG_OPTIONS_EXIT) {
        return EXIT_SUCCESS;
    }

    // Set the lists of allowed paths for cases where a path comes from an
    // untrusted source, such as the global property tree (this uses $FG_HOME
    // and other paths set by Options::processOptions()).
    fgInitAllowedPaths();

    const auto& resMgr = simgear::EmbeddedResourceManager::createInstance();
    initFlightGearEmbeddedResources();
    // The language was set in processOptions()
    const std::string locale = globals->get_locale()->getPreferredLanguage();
    // Must always be done after all resources have been added to 'resMgr'
    resMgr->selectLocale(locale);
    SG_LOG(SG_GENERAL, SG_INFO,
           "EmbeddedResourceManager: selected locale '" << locale << "'");

    // Copy the property nodes for the menus added by registered add-ons
    addons::AddonManager::instance()->addAddonMenusToFGMenubar();

    // Initialize the Window/Graphics environment.
    fgOSInit(&argc, argv);
    _bootstrap_OSInit++;

    fgRegisterIdleHandler( &fgIdleFunction );

    // Initialize sockets (WinSock needs this)
    simgear::Socket::initSockets();

    // Clouds3D requires an alpha channel
    fgOSOpenWindow(true /* request stencil buffer */);
    fgOSResetProperties();

    fntInit();
    globals->get_renderer()->preinit();

    if (fgGetBool("/sim/ati-viewport-hack", true)) {
        SG_LOG(SG_GENERAL, SG_WARN, "Enabling ATI/AMD viewport hack");
        ATIScreenSizeHack();
    }

    fgOutputSettings();

    //try to disable the screensaver
    fgOSDisableScreensaver();

    // pass control off to the master event handler
    int result = fgOSMainLoop();
    frame_signal.clear();
    fgOSCloseWindow();

    simgear::clearEffectCache();

    // clean up here; ensure we null globals to avoid
    // confusing the atexit() handler
    delete globals;
    globals = NULL;

    // delete the NavCache here. This will cause the destruction of many cached
    // objects (eg, airports, navaids, runways).
    delete flightgear::NavDataCache::instance();

    return result;
}
