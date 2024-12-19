/*
 * SPDX-FileName: main.cxx
 * SPDX-FileComment: top level sim routines
 * SPDX-FileCopyrightText: Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <simgear/compiler.h>
#include <simgear/props/props_io.hxx>

#include <iostream>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osgDB/Registry>


// Class references
#include <simgear/canvas/VGInitOperation.hxx>
#include <simgear/debug/logdelta.hxx>
#include <simgear/emesary/Emesary.hxx>
#include <simgear/emesary/notifications.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/nasal/NasalEmesaryInterface.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/tsync/terrasync.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Add-ons/AddonManager.hxx>
#include <GUI/MessageBox.hxx>
#include <GUI/gui.h>
#include <Main/locale.hxx>
#include <Model/panelnode.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Scenery/scenery.hxx>
#include <Sound/soundmanager.hxx>
#include <Time/TimeManager.hxx>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/GraphicsPresets.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/splash.hxx>
#include <flightgearBuildId.h>

#include "fg_commands.hxx"
#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "main.hxx"
#include "options.hxx"
#include "positioninit.hxx"
#include "screensaver_control.hxx"
#include "subsystemFactory.hxx"
#include "util.hxx"
#include <Main/ErrorReporter.hxx>
#include <Main/sentryIntegration.hxx>

#include <simgear/embedded_resources/EmbeddedResourceManager.hxx>
#include <EmbeddedResources/FlightGear-resources.hxx>

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

#ifdef __OpenBSD__
#include <sys/resource.h>
#endif

using namespace flightgear;

using std::cerr;
using std::vector;

// The atexit() function handler should know when the graphical subsystem
// is initialized.
extern int _bootstrap_OSInit;

static SGPropertyNode_ptr frame_signal;

#ifdef NASAL_BACKGROUND_GC_THREAD
static SGPropertyNode_ptr nasal_gc_threaded;
static SGPropertyNode_ptr nasal_gc_threaded_wait;
static  SGSharedPtr<simgear::Notifications::MainLoopNotification> mln_begin(new simgear::Notifications::MainLoopNotification(simgear::Notifications::MainLoopNotification::Type::Begin));
static  SGSharedPtr<simgear::Notifications::MainLoopNotification> mln_end(new simgear::Notifications::MainLoopNotification(simgear::Notifications::MainLoopNotification::Type::End));
static  SGSharedPtr<simgear::Notifications::MainLoopNotification> mln_started(new simgear::Notifications::MainLoopNotification(simgear::Notifications::MainLoopNotification::Type::Started));
static  SGSharedPtr<simgear::Notifications::MainLoopNotification> mln_stopped(new simgear::Notifications::MainLoopNotification(simgear::Notifications::MainLoopNotification::Type::Stopped));
static  SGSharedPtr<simgear::Notifications::NasalGarbageCollectionConfigurationNotification> ngccn;
#endif
// This method is usually called after OSG has finished rendering a frame in what OSG calls an idle handler and
// is reposonsible for invoking all of the relevant per frame processing; most of which is handled by subsystems.
static void fgMainLoop( void )
{
#ifdef NASAL_BACKGROUND_GC_THREAD
    //
    // the Nasal GC will automatically run when (during allocation) it discovers that more space is needed.
    // This has a cost of between 5ms and 50ms (depending on the amount of currently active Nasal).
    // The result is unscheduled and unpredictable pauses during normal operation when the garbage collector
    // runs; which typically occurs at intervals between 1sec and 20sec.
    // 
    // The solution to this, which overall increases CPU load, is to use a thread to do this; as Nasal is thread safe
    // so what we do is to launch the garbage collection at the end of the main loop and then wait for completion at the start of the
    // next main loop.
    // So although the overall CPU is increased it has little effect on the frame rate; if anything it is an overall benefit
    // as there are no unscheduled long duration frames.
    //
    // The implementation appears to work fine without waiting for completion at the start of the frame - so
    // this wait at the start can be disabled by setting the property /sim/nasal-gc-threaded-wait to false.

    // first we see if the config has changed. The notification will return true from SetActive/SetWait when the
    // value has been changed - and thus we notify the Nasal system that it should configure itself accordingly.
    auto use_threaded_gc = nasal_gc_threaded->getBoolValue();
    auto threaded_wait = nasal_gc_threaded_wait->getBoolValue();
    bool notify_gc_config = false;
    notify_gc_config = ngccn->SetActive(use_threaded_gc);
    notify_gc_config |= ngccn->SetWait(threaded_wait);
    if (notify_gc_config)
         simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(ngccn);

     simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(mln_begin);
#endif

    if (sglog().has_popup()) {
        std::string s = sglog().get_popup();
        flightgear::modalMessageBox("Alert", s, "");
    }

    frame_signal->fireValueChanged();

    // Fetch the subsystem manager.
    auto mgr = globals->get_subsystem_mgr();

    // compute simulated time (allowing for pause, warp, etc) and
    // real elapsed time
    double sim_dt, real_dt;
    mgr->get_subsystem<TimeManager>()->computeTimeDeltas(sim_dt, real_dt);

    // update all subsystems
    mgr->update(sim_dt);

    // flush commands waiting in the queue
    SGCommandMgr::instance()->executedQueuedCommands();
    simgear::AtomicChangeListener::fireChangeListeners();

#ifdef NASAL_BACKGROUND_GC_THREAD
    simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(mln_end);
#endif
}

static void initTerrasync()
{
    // add the terrasync root as a data path so data can be retrieved from it
    // (even if we are in read-only mode)
    SGPath terraSyncDir(globals->get_terrasync_dir());
    globals->append_data_path(terraSyncDir, false /* = ahead of FG_ROOT */);

    if (fgGetBool("/sim/fghome-readonly", false)) {
        SG_LOG(SG_GENERAL, SG_ALERT, "initTerrasync() failing because /sim/fghome-readonly is true");
        return;
    }

    // make fg-root dir available so existing Scenery data can be copied, and
    // hence not downloaded again.
    fgSetString("/sim/terrasync/installation-dir", (globals->get_fg_root() / "Scenery").utf8Str());

    auto terra_sync = globals->get_subsystem_mgr()->add<simgear::SGTerraSync>();
    terra_sync->setRoot(globals->get_props());

    terra_sync->bind();
    terra_sync->init();
    
    if (fgGetBool("/sim/terrasync/enabled")) {
        flightgear::addSentryTag("terrasync", "enabled");
    }
}

static void fgSetVideoOptions()
{
    SGPath userDataPath = globals->get_fg_home();
    SGPath autosaveFile = globals->autosaveFilePath(userDataPath);
    if (autosaveFile.exists()) return;

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
        }
    }
}

static void checkOpenGLVersion()
{
    flightgear::addSentryTag("gl-version", fgGetString("/sim/rendering/gl-version"));
    flightgear::addSentryTag("gl-renderer", fgGetString("/sim/rendering/gl-vendor"));
    flightgear::addSentryTag("gl-vendor", fgGetString("/sim/rendering/gl-renderer"));
    
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

namespace flightgear {

void registerMainLoop()
{
    // stash current frame signal property
    frame_signal = fgGetNode("/sim/signals/frame", true);

#ifdef NASAL_BACKGROUND_GC_THREAD
    nasal_gc_threaded = fgGetNode("/sim/nasal-gc-threaded", true);
    nasal_gc_threaded_wait = fgGetNode("/sim/nasal-gc-threaded-wait", true);
#endif
    // init the Emesary receiver for Nasal
    nasal::initMainLoopRecipient();

    fgRegisterIdleHandler( fgMainLoop );
}

void unregisterMainLoopProperties()
{
    nasal::shutdownMainLoopRecipient();
    frame_signal.reset();
#ifdef NASAL_BACKGROUND_GC_THREAD
    nasal_gc_threaded.reset();
    nasal_gc_threaded_wait.reset();
#endif
}

} // namespace flightgear

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

    // Fetch the subsystem manager.
    auto mgr = globals->get_subsystem_mgr();

    if ( idle_state == 0 ) {
        auto camera = flightgear::getGUICamera(flightgear::CameraGroup::getDefault());
        if (guiInit(camera->getGraphicsContext())) {
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

        mgr->add<TimeManager>();

        // Do some quick general initializations
        if( !fgInitGeneral()) {
            throw sg_exception("General initialization failed");
        }

        // now we have commands up
        flightgear::delayedSentryInit();

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

        simgear::SGModelLib::init(globals->get_fg_root().utf8Str(), globals->get_props());

        auto timeManager = mgr->get_subsystem<TimeManager>();
        timeManager->init();

        ////////////////////////////////////////////////////////////////////
        // Initialize the TG scenery subsystem.
        ////////////////////////////////////////////////////////////////////

        auto scenery = mgr->add<FGScenery>();
        scenery->init();
        scenery->bind();

        fgSplashProgress("creating-subsystems");
    } else if (( idle_state == 7 ) || (idle_state == 2007)) {
        bool isReset = (idle_state == 2007);
        idle_state = 8; // from the next state on, reset & startup are identical
        SGTimeStamp st;
        st.stamp();

        try {
            fgCreateSubsystems(isReset);
        } catch (std::exception& e) {
            // attempt to trace location of illegal argument / invalid string
            // position errors on startup
            flightgear::sentryReportException(std::string{"Creating subsystems: caught:"} + e.what());
            throw;
        }

        SG_LOG(SG_GENERAL, SG_INFO, "Creating subsystems took:" << st.elapsedMSec());
        fgSplashProgress("binding-subsystems");

    } else if ( idle_state == 8 ) {
        idle_state++;
        SGTimeStamp st;
        st.stamp();
        mgr->bind();
        SG_LOG(SG_GENERAL, SG_INFO, "Binding subsystems took:" << st.elapsedMSec());

        fgSplashProgress("init-subsystems");
    } else if ( idle_state == 9 ) {
        SGSubsystem::InitStatus status = mgr->incrementalInit();
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
        flightgear::registerMainLoop();

#ifdef NASAL_BACKGROUND_GC_THREAD
        ngccn = new simgear::Notifications::NasalGarbageCollectionConfigurationNotification(nasal_gc_threaded->getBoolValue(), nasal_gc_threaded_wait->getBoolValue());
         simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(ngccn);
         simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(mln_started);
#endif        
        flightgear::addSentryBreadcrumb("entering main loop", "info");
    }

    if ( idle_state == 2000 ) {
        flightgear::addSentryBreadcrumb("starting reset", "info");
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
        SG_LOG(SG_GENERAL, SG_MANDATORY_INFO, "\n!! Network connections allowed to use Nasal !!\n"
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

static void rotateOldLogFiles()
{
    const int maxLogCount = 10;
    const auto homePath = globals->get_fg_home();

    for (int i = maxLogCount; i > 0; --i) {
        const auto name = "fgfs_" + std::to_string(i - 1) + ".log";
        SGPath curLogFile = homePath / name;
        if (curLogFile.exists()) {
            auto newName = "fgfs_" + std::to_string(i) + ".log";
            curLogFile.rename(homePath / newName);
        }
    }

    SGPath p = homePath / "fgfs.log";
    if (!p.exists())
        return;
    SGPath log0Path = homePath / "fgfs_0.log";
    if (!p.rename(log0Path)) {
        std::cerr << "Failed to rename " << p.str() << " to " << log0Path.str() << std::endl;
    }
}

static void logToHome(const std::string& pri)
{
    sgDebugPriority fileLogLevel = SG_INFO;
    // https://sourceforge.net/p/flightgear/codetickets/2100/
    if (!pri.empty()) {
        try {
            fileLogLevel = std::min(fileLogLevel, logstream::priorityFromString(pri));
        } catch (std::exception& ) {
            // let's not worry about this, and just log at INFO
        }
    }

    SGPath logPath = globals->get_fg_home();
    logPath.append("fgfs.log");
    if (logPath.exists()) {
        rotateOldLogFiles();
    }

    sglog().logToFile(logPath, SG_ALL, fileLogLevel);
}

struct SGLogDeltasListener : SGPropertyChangeListener
{
    void valueChanged(SGPropertyNode* node) override
    {
        std::string value = node->getStringValue();
        std::cerr << __FILE__ << ":" << __LINE__ << ": sglogdeltas value=" << value << "\n";
        logDeltaSet(value.c_str());
    }
};
static SGLogDeltasListener s_sglogdeltas_listener;

// Main top level initialization
int fgMainInit( int argc, char **argv )
{
    sglog().setLogLevels( SG_ALL, SG_WARN );
    sglog().setStartupLoggingEnabled(true);
    
    globals = new FGGlobals;
    auto initHomeResult = fgInitHome();
    if (initHomeResult == InitHomeAbort) {
        flightgear::fatalMessageBoxThenExit("Unable to create lock file",
                                "Flightgear was unable to create the lock file in FG_HOME");
    }
    
#if defined(HAVE_QT)
    flightgear::initApp(argc, argv);
#endif

    // check if the launcher is requested, since it affects config file parsing
    bool showLauncher = flightgear::Options::checkForArgEnable(argc, argv, "launcher");
    // an Info.plist bundle can't define command line arguments, but it can set
    // environment variables. This avoids needed a wrapper shell-script on OS-X.
    showLauncher |= (::getenv("FG_LAUNCHER") != nullptr);

    bool needToShowReadOnlyWarning = (initHomeResult == InitHomeReadOnly);
#if defined(HAVE_QT)
    // honour headless mode, even if Qt is enabled
    // if we're in headless mode, we fall through to the case
    // below and call modalMessageBox which logs
    if (needToShowReadOnlyWarning && !flightgear::isHeadlessMode()) {
        needToShowReadOnlyWarning = false;
        auto r = flightgear::showLockFileDialog();
        if (r == flightgear::LockFileReset) {
            SG_LOG( SG_GENERAL, SG_MANDATORY_INFO, "Deleting lock file at user request");
            flightgear::addSentryBreadcrumb("deleting lock-file at user request", "info");
            fgDeleteLockFile();
            fgSetBool("/sim/fghome-readonly", false);
        } else if (r == flightgear::LockFileQuit) {
            return EXIT_SUCCESS;
        }
    }
 #endif

    if (needToShowReadOnlyWarning) {
        // will handle headless mode by logging
        flightgear::modalMessageBox("Multiple copies of FlightGear",
            "Another copy of FlightGear is running, so this copy will run read-only.", 
            "This means aircraft and scenery cannot be downloaded, and settings will not be saved.");
    }

    {
        SGPropertyNode* sglogdeltas = globals->get_props()->getNode("/sim/sg-log-deltas", true /*create*/);
        assert(sglogdeltas);
        sglogdeltas->addChangeListener(&s_sglogdeltas_listener, false /*initial*/);
        const char* sglogdeltas_value = getenv("SG_LOG_DELTAS");
        if (sglogdeltas_value) {
            sglogdeltas->setStringValue(sglogdeltas_value);
        }
    }
    
    globals->get_props()->getNode("/sim", true /*create*/)->setAttribute(SGPropertyNode::VALUE_CHANGED_DOWN, true);
    
    {
        SGPropertyNode* active = globals->get_props()->getNode("/sim/property-locking/active", true /*create*/);
        SGPropertyNode* verbose = globals->get_props()->getNode("/sim/property-locking/verbose", true /*create*/);
        SGPropertyNode* timing = globals->get_props()->getNode("/sim/property-locking/timing", true /*create*/);
        SGPropertyNode* parent_listeners = globals->get_props()->getNode("/sim/property-locking/parent_listeners", true /*create*/);
        SGPropertyLockControl(active, verbose, timing, parent_listeners);
    }
    
    const bool readOnlyFGHome = fgGetBool("/sim/fghome-readonly");
    if (!readOnlyFGHome) {
        // now home is initialised, we can log to a file inside it
        const auto level = flightgear::Options::getArgValue(argc, argv, "--log-level");
        logToHome(level);
    }

    if (readOnlyFGHome) {
        flightgear::addSentryTag("fghome-readonly", "true");
    }

    std::string version(FLIGHTGEAR_VERSION);
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Version " << version );
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Build Type " << FG_BUILD_TYPE );
    SG_LOG( SG_GENERAL, SG_INFO, "Built with " << SG_COMPILER_STR);
	SG_LOG( SG_GENERAL, SG_INFO, "Jenkins number/ID " << JENKINS_BUILD_NUMBER << ":"
			<< JENKINS_BUILD_ID);

    flightgear::addSentryTag("osg-version", osgGetVersion());
    
#ifdef __OpenBSD__
    {
        /* OpenBSD defaults to a small maximum data segment, which can cause
        flightgear to crash with SIGBUS, so output a warning if this is likely.
        */
        struct rlimit   rlimit;
        int e = getrlimit(RLIMIT_DATA, &rlimit);
        if (e) {
            SG_LOG( SG_GENERAL, SG_INFO, "This is OpenBSD; getrlimit() failed: " << strerror(errno));
        }
        else {
            unsigned long long   required = 4ULL * (1ULL<<30);
            if (rlimit.rlim_cur < required) {
                SG_LOG( SG_GENERAL, SG_POPUP, ""
                        << "Max data segment (" << rlimit.rlim_cur << "bytes) too small.\n"
                        << "This can cause Flightgear to crash due to SIGBUS.\n"
                        << "E.g. increase with 'ulimit -d " << required/1024 << "'."
                        );
            }
        }
    }
#endif

    // seed the random number generator
    sg_srandom_time();

    string_list *col = new string_list;
    globals->set_channel_options_list( col );

    if (showLauncher) {
        // to minimise strange interactions when launcher and config files
        // set overlaping options, we disable the default files. Users can
        // still explicitly request config files via --config options if they choose.
        flightgear::Options::sharedInstance()->setShouldLoadDefaultConfig(false);
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

    bool didUseLauncher = false;
#if defined(HAVE_QT)
    if (showLauncher) {
        flightgear::addSentryBreadcrumb("starting launcher", "info");
        if (!flightgear::runLauncherDialog()) {
            return EXIT_SUCCESS;
        }

        didUseLauncher = true;
        flightgear::addSentryBreadcrumb("completed launcher", "info");
    }
#else
    if (showLauncher) {
        SG_LOG(SG_GENERAL, SG_ALERT, "\n!Launcher requested, but FlightGear was compiled without Qt support!\n");
    }
#endif
    
    fgInitSecureMode();
    fgInitAircraftPaths(false);

    auto errorManager = globals->get_subsystem_mgr()->add<flightgear::ErrorReporter>();
    errorManager->preinit();

    configResult = fgInitAircraft(false, didUseLauncher);
    if (configResult == flightgear::FG_OPTIONS_ERROR) {
        return EXIT_FAILURE;
    } else if ((configResult == flightgear::FG_OPTIONS_EXIT) ||
               (configResult == flightgear::FG_OPTIONS_SHOW_AIRCRAFT))
    {
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

    if (fgGetBool("/sim/autosave-migration/did-migrate", false)) {
        // inform the user we did migration. This is the earliest point
        // we can do it, since now the locale is set
        auto locale = globals->get_locale();
        const auto title = locale->getLocalizedString("settings-migration-title", "sys", "Settings migrated");
        const auto msg = locale->getLocalizedString("settings-migration-text", "sys",
                                                    "Saved settings were migrated from a previous version of FlightGear. "
                                                    "If you encounter any problems when using the system, try restoring "
                                                    "the default settings, before reporting a problem. "
                                                    "Saved settings can affect the appearance, performance and features of the simulator.");
        flightgear::modalMessageBox(title, msg);
    }

    // Copy the property nodes for the menus added by registered add-ons
    addons::AddonManager::instance()->addAddonMenusToFGMenubar();

    // The GraphicsPreset subsystem is special - it's not added together with
    // the rest of the subsystems because it should be present before all the
    // graphics-related stuff is initialized.
    auto presets = globals->get_subsystem_mgr()->add<flightgear::GraphicsPresets>();
    presets->applyInitialPreset();

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
        flightgear::addSentryTag("ati-viewport-hack", "enabled");
        ATIScreenSizeHack();
    }

    fgOutputSettings();

    //try to disable the screensaver
    fgOSDisableScreensaver();

    // pass control off to the master event handler
    int result = fgOSMainLoop();
    flightgear::unregisterMainLoopProperties();

    fgOSCloseWindow();
    fgShutdownHome();

    const bool requestLauncherRestart = fgGetBool("/sim/restart-launcher-on-exit");

#ifdef NASAL_BACKGROUND_GC_THREAD
    simgear::Emesary::GlobalTransmitter::instance()->NotifyAll(mln_stopped);
#endif

    simgear::clearEffectCache();

    // clean up here; ensure we null globals to avoid
    // confusing the atexit() handler
    delete globals;
    globals = nullptr;

    // delete the NavCache here. This will cause the destruction of many cached
    // objects (eg, airports, navaids, runways).
    delete flightgear::NavDataCache::instance();

#if defined(HAVE_QT)
    if (requestLauncherRestart) {
        string_list originalArgs;
        for (int arg = 1; arg < argc; ++arg) {
            originalArgs.push_back(argv[arg]);
        }

        if (!showLauncher) {
            // The user specified the --restart-launcher option but without the
            // --launcher option. So let's add the --launcher option so that the
            // launcher actually starts instead of the simulator again.
            originalArgs.push_back("--launcher");
        }

        flightgear::addSentryBreadcrumb("Requested to restart launcher", "info");
        flightgear::startLaunchOnExit(originalArgs);
    }
#endif
    return result;
}
