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

#if defined(__linux__) && defined(__i386__)
#  include <fpu_control.h>
#  include <signal.h>
#endif

#include <iostream>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osgDB/Registry>

// Class references
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/scene/tsync/terrasync.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_random.h>

#include <Aircraft/controls.hxx>
#include <Model/panelnode.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Sound/fg_fx.hxx>
#include <Time/TimeManager.hxx>
#include <GUI/gui.h>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <Viewer/splash.hxx>
#include <Viewer/renderer.hxx>

#include "fg_commands.hxx"
#include "fg_io.hxx"
#include "main.hxx"
#include "util.hxx"
#include "fg_init.hxx"
#include "fg_os.hxx"
#include "fg_props.hxx"

using namespace flightgear;

using std::cerr;
using std::vector;

// Specify our current idle function state.  This is used to run all
// our initializations out of the idle callback so that we can get a
// splash screen up and running right away.
int idle_state = 0;

// The atexit() function handler should know when the graphical subsystem
// is initialized.
extern int _bootstrap_OSInit;


void fgInitSoundManager()
{
    SGSoundMgr *smgr = globals->get_soundmgr();

    smgr->bind();
    smgr->init(fgGetString("/sim/sound/device-name", NULL));

    vector <const char*>devices = smgr->get_available_devices();
    for (unsigned int i=0; i<devices.size(); i++) {
        SGPropertyNode *p = fgGetNode("/sim/sound/devices/device", i, true);
        p->setStringValue(devices[i]);
    }
    devices.clear();
}

void fgSetNewSoundDevice(const char *device)
{
    SGSoundMgr *smgr = globals->get_soundmgr();
    smgr->suspend();
    smgr->stop();
    smgr->init(device);
    smgr->resume();
}

// Update sound manager state (init/suspend/resume) and propagate property values,
// since the sound manager doesn't read any properties itself.
// Actual sound update is triggered by the subsystem manager.
static void fgUpdateSound(double dt)
{
#ifdef ENABLE_AUDIO_SUPPORT
    static bool smgr_init = true;
    static SGPropertyNode *sound_working = fgGetNode("/sim/sound/working");
    if (smgr_init == true) {
        if (sound_working->getBoolValue() == true) {
            fgInitSoundManager();
            smgr_init = false;
        }
    } else {
        static SGPropertyNode *sound_enabled = fgGetNode("/sim/sound/enabled");
        static SGSoundMgr *smgr = globals->get_soundmgr();
        static bool smgr_enabled = true;

        if (sound_working->getBoolValue() == false) {   // request to reinit
           smgr->reinit();
           smgr->resume();
           sound_working->setBoolValue(true);
        }

        if (smgr_enabled != sound_enabled->getBoolValue()) {
            if (smgr_enabled == true) { // request to suspend
                smgr->suspend();
                smgr_enabled = false;
            } else {
                smgr->resume();
                smgr_enabled = true;
            }
        }

        if (smgr_enabled == true) {
            static SGPropertyNode *volume = fgGetNode("/sim/sound/volume");
            smgr->set_volume(volume->getFloatValue());
        }
    }
#endif
}

static void fgLoadInitialScenery()
{
    static SGPropertyNode_ptr scenery_loaded
        = fgGetNode("sim/sceneryloaded", true);

    if (!scenery_loaded->getBoolValue())
    {
        if (globals->get_tile_mgr()->isSceneryLoaded()
             && fgGetBool("sim/fdm-initialized")) {
            fgSetBool("sim/sceneryloaded",true);
            fgSplashProgress("");
            if (fgGetBool("/sim/sound/working")) {
                globals->get_soundmgr()->activate();
            }
            globals->get_props()->tie("/sim/sound/devices/name",
                  SGRawValueFunctions<const char *>(0, fgSetNewSoundDevice), false);
        }
        else
        {
            fgSplashProgress("loading scenery");
            // be nice to loader threads while waiting for initial scenery, reduce to 2fps
            SGTimeStamp::sleepForMSec(500);
        }
    }
}

// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void )
{
    static SGPropertyNode_ptr frame_signal
        = fgGetNode("/sim/signals/frame", true);

    frame_signal->fireValueChanged();

    SG_LOG( SG_GENERAL, SG_DEBUG, "Running Main Loop");
    SG_LOG( SG_GENERAL, SG_DEBUG, "======= ==== ====");

    // compute simulated time (allowing for pause, warp, etc) and
    // real elapsed time
    double sim_dt, real_dt;
    TimeManager* timeMgr = (TimeManager*) globals->get_subsystem("time");
    timeMgr->computeTimeDeltas(sim_dt, real_dt);

    // update magvar model
    globals->get_mag()->update( globals->get_aircraft_position(),
                                globals->get_time_params()->getJD() );

    // Propagate sound manager properties (note: actual update is triggered
    // by the subsystem manager).
    fgUpdateSound(sim_dt);

    // update all subsystems
    globals->get_subsystem_mgr()->update(sim_dt);

    // END Tile Manager updates
    fgLoadInitialScenery();

    simgear::AtomicChangeListener::fireChangeListeners();

    SG_LOG( SG_GENERAL, SG_DEBUG, "" );
}

// Operation for querying OpenGL parameters. This must be done in a
// valid OpenGL context, potentially in another thread.
namespace
{
struct GeneralInitOperation : public GraphicsContextOperation
{
    GeneralInitOperation()
        : GraphicsContextOperation(std::string("General init"))
    {
    }
    void run(osg::GraphicsContext* gc)
    {
        SGPropertyNode* simRendering = fgGetNode("/sim/rendering");
        
        simRendering->setStringValue("gl-vendor", (char*) glGetString(GL_VENDOR));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_VENDOR));
        
        simRendering->setStringValue("gl-renderer", (char*) glGetString(GL_RENDERER));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_RENDERER));
        
        simRendering->setStringValue("gl-version", (char*) glGetString(GL_VERSION));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_VERSION));

        GLint tmp;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &tmp );
        simRendering->setIntValue("max-texture-size", tmp);

        glGetIntegerv( GL_DEPTH_BITS, &tmp );
        simRendering->setIntValue("depth-buffer-bits", tmp);
    }
};

}

// This is the top level master main function that is registered as
// our idle function

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static void fgIdleFunction ( void ) {
    static osg::ref_ptr<GeneralInitOperation> genOp;
    if ( idle_state == 0 ) {
        idle_state++;
        // Pick some window on which to do queries.
        // XXX Perhaps all this graphics initialization code should be
        // moved to renderer.cxx?
        genOp = new GeneralInitOperation;
        osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
        WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
        osg::GraphicsContext* gc = 0;
        if (guiCamera)
            gc = guiCamera->getGraphicsContext();
        if (gc) {
            gc->add(genOp.get());
        } else {
            wsa->windows[0]->gc->add(genOp.get());
        }
        guiStartInit(gc);
    } else if ( idle_state == 1 ) {
        if (genOp.valid()) {
            if (!genOp->isFinished())
                return;
            genOp = 0;
        }
        if (!guiFinishInit())
            return;
        idle_state++;
        fgSplashProgress("loading aircraft list");

    } else if ( idle_state == 2 ) {
        idle_state++;
        fgSplashProgress("loading navigation data");

    } else if ( idle_state == 3 ) {
        idle_state++;
        fgInitNav();

        fgSplashProgress("initializing scenery system");

    } else if ( idle_state == 4 ) {
        idle_state++;
        // based on the requested presets, calculate the true starting
        // lon, lat
        fgInitPosition();
        fgInitTowerLocationListener();

        TimeManager* t = new TimeManager;
        globals->add_subsystem("time", t, SGSubsystemMgr::INIT);
        t->init(); // need to init now, not during initSubsystems
        
        // Do some quick general initializations
        if( !fgInitGeneral()) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                "General initialization failed ..." );
            exit(-1);
        }

        ////////////////////////////////////////////////////////////////////
        // Initialize the property-based built-in commands
        ////////////////////////////////////////////////////////////////////
        fgInitCommands();

        ////////////////////////////////////////////////////////////////////
        // Initialize the material manager
        ////////////////////////////////////////////////////////////////////
        globals->set_matlib( new SGMaterialLib );
        simgear::SGModelLib::init(globals->get_fg_root(), globals->get_props());
        simgear::SGModelLib::setPanelFunc(FGPanelNode::load);

        ////////////////////////////////////////////////////////////////////
        // Initialize the TG scenery subsystem.
        ////////////////////////////////////////////////////////////////////
        simgear::SGTerraSync* terra_sync = new simgear::SGTerraSync(globals->get_props());
        globals->add_subsystem("terrasync", terra_sync);
        globals->set_scenery( new FGScenery );
        globals->get_scenery()->init();
        globals->get_scenery()->bind();
        globals->set_tile_mgr( new FGTileMgr );

        fgSplashProgress("loading aircraft");

    } else if ( idle_state == 5 ) {
        idle_state++;

        fgSplashProgress("initializing sky elements");

    } else if ( idle_state == 6 ) {
        idle_state++;
        
        // Initialize MagVar model
        SGMagVar *magvar = new SGMagVar();
        globals->set_mag( magvar );
        
        
        // kludge to initialize mag compass
        // (should only be done for in-flight
        // startup)
        // update magvar model
        globals->get_mag()->update( fgGetDouble("/position/longitude-deg")
                                   * SGD_DEGREES_TO_RADIANS,
                                   fgGetDouble("/position/latitude-deg")
                                   * SGD_DEGREES_TO_RADIANS,
                                   fgGetDouble("/position/altitude-ft")
                                   * SG_FEET_TO_METER,
                                   globals->get_time_params()->getJD() );

        fgSplashProgress("initializing subsystems");

    } else if ( idle_state == 7 ) {
        idle_state++;
        // Initialize audio support
#ifdef ENABLE_AUDIO_SUPPORT

        // Start the intro music
        if ( fgGetBool("/sim/startup/intro-music") ) {
            SGPath mp3file( globals->get_fg_root() );
            mp3file.append( "Sounds/intro.mp3" );

            SG_LOG( SG_GENERAL, SG_INFO,
                "Starting intro music: " << mp3file.str() );

# if defined( __CYGWIN__ )
            string command = "start /m `cygpath -w " + mp3file.str() + "`";
# elif defined( _WIN32 )
            string command = "start /m " + mp3file.str();
# else
            string command = "mpg123 " + mp3file.str() + "> /dev/null 2>&1";
# endif

            if (0 != system ( command.c_str() ))
            {
                SG_LOG( SG_SOUND, SG_WARN,
                    "Failed to play mp3 file " << mp3file.str() << ". Maybe mp3 player is not installed." );
            }
        }
#endif
        // This is the top level init routine which calls all the
        // other subsystem initialization routines.  If you are adding
        // a subsystem to flightgear, its initialization call should be
        // located in this routine.
        if( !fgInitSubsystems()) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                "Subsystem initialization failed ..." );
            exit(-1);
        }

        // Torsten Dreyer:
        // ugly hack for automatic runway selection on startup based on
        // metar data. Makes startup.nas obsolete and guarantees the same
        // runway selection as for AI traffic. However, this code belongs to
        // somewhere(?) else - if I only new where...
        if( true == fgGetBool( "/environment/metar/valid" ) ) {
            SG_LOG(SG_ENVIRONMENT, SG_INFO,
                "Using METAR for runway selection: '" << fgGetString("/environment/metar/data") << "'" );
            // the realwx_ctrl fetches metar in the foreground on init,
            // If it was able to fetch a metar or one was given on the commandline,
            // the valid flag is set here, otherwise it is false
            double hdg = fgGetDouble( "/environment/metar/base-wind-dir-deg", 9999.0 );
            string apt = fgGetString( "/sim/startup/options/airport" );
            string rwy = fgGetString( "/sim/startup/options/runway" );
            double strthdg = fgGetDouble( "/sim/startup/options/heading-deg", 9999.0 );
            string parkpos = fgGetString( "/sim/presets/parkpos" );
            bool onground = fgGetBool( "/sim/presets/onground", false );
            // don't check for wind-speed < 1kt, this belongs to the runway-selection code
            // the other logic is taken from former startup.nas
            if( hdg < 360.0 && apt.length() > 0 && strthdg > 360.0 && rwy.length() == 0 && onground && parkpos.length() == 0 ) {
                extern bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg );
                fgSetPosFromAirportIDandHdg( apt, hdg );
            }
        } else {
            SG_LOG(SG_ENVIRONMENT, SG_INFO,
                "No METAR available to pick active runway" );
        }

        fgSplashProgress("initializing graphics engine");

    } else if ( idle_state == 8 ) {
        idle_state = 1000;
        
        // setup OpenGL view parameters
        globals->get_renderer()->setupView();

        globals->get_renderer()->resize( fgGetInt("/sim/startup/xsize"),
                                         fgGetInt("/sim/startup/ysize") );

        int session = fgGetInt("/sim/session",0);
        session++;
        fgSetInt("/sim/session",session);
    }

    if ( idle_state == 1000 ) {
        // We've finished all our initialization steps, from now on we
        // run the main loop.
        fgSetBool("sim/sceneryloaded", false);
        fgRegisterIdleHandler( fgMainLoop );
    }
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
    p->addChangeListener(new FGMakeUpperCase);
}


// Main top level initialization
int fgMainInit( int argc, char **argv ) {

    // set default log levels
    sglog().setLogLevels( SG_ALL, SG_ALERT );

    string version;
#ifdef FLIGHTGEAR_VERSION
    version = FLIGHTGEAR_VERSION;
#else
    version = "unknown version";
#endif
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Version "
            << version );
    SG_LOG( SG_GENERAL, SG_INFO, "Built with " << SG_COMPILER_STR << std::endl );

    // Allocate global data structures.  This needs to happen before
    // we parse command line options

    globals = new FGGlobals;

    // seed the random number generator
    sg_srandom_time();

    FGControls *controls = new FGControls;
    globals->set_controls( controls );

    string_list *col = new string_list;
    globals->set_channel_options_list( col );

    fgValidatePath("", false);  // initialize static variables
    upper_case_property("/sim/presets/airport-id");
    upper_case_property("/sim/presets/runway");
    upper_case_property("/sim/tower/airport-id");
    upper_case_property("/autopilot/route-manager/input");

    // Load the configuration parameters.  (Command line options
    // override config file options.  Config file options override
    // defaults.)
    if ( !fgInitConfig(argc, argv) ) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Config option parsing failed ..." );
      exit(-1);
    }

    // Initialize the Window/Graphics environment.
    fgOSInit(&argc, argv);
    _bootstrap_OSInit++;

    fgRegisterIdleHandler( &fgIdleFunction );

    // Initialize sockets (WinSock needs this)
    simgear::Socket::initSockets();

    // Clouds3D requires an alpha channel
    fgOSOpenWindow(true /* request stencil buffer */);

    // Initialize the splash screen right away
    fntInit();
    fgSplashInit();

    // pass control off to the master event handler
    int result = fgOSMainLoop();
    
    // clean up here; ensure we null globals to avoid
    // confusing the atexit() handler
    delete globals;
    globals = NULL;
    
    return result;
}
