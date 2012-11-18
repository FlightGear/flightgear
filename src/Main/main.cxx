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

#include <iostream>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osgDB/Registry>

// Class references
#include <simgear/canvas/VGInitOperation.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/scene/tsync/terrasync.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_random.h>

#include <Model/panelnode.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Sound/soundmanager.hxx>
#include <Time/TimeManager.hxx>
#include <GUI/gui.h>
#include <Viewer/splash.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <Navaids/NavDataCache.hxx>

#include "fg_commands.hxx"
#include "fg_io.hxx"
#include "main.hxx"
#include "util.hxx"
#include "fg_init.hxx"
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "positioninit.hxx"
#include "subsystemFactory.hxx"

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
    frame_signal->fireValueChanged();

    SG_LOG( SG_GENERAL, SG_DEBUG, "Running Main Loop");
    SG_LOG( SG_GENERAL, SG_DEBUG, "======= ==== ====");

    // compute simulated time (allowing for pause, warp, etc) and
    // real elapsed time
    double sim_dt, real_dt;
    static TimeManager* timeMgr = (TimeManager*) globals->get_subsystem("time");
    timeMgr->computeTimeDeltas(sim_dt, real_dt);

    // update all subsystems
    globals->get_subsystem_mgr()->update(sim_dt);

    simgear::AtomicChangeListener::fireChangeListeners();

    SG_LOG( SG_GENERAL, SG_DEBUG, "" );
}


// This is the top level master main function that is registered as
// our idle function

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static void fgIdleFunction ( void ) {
    // Specify our current idle function state.  This is used to run all
    // our initializations out of the idle callback so that we can get a
    // splash screen up and running right away.
    static int idle_state = 0;
  
    if ( idle_state == 0 ) {
        if (guiInit())
        {
            idle_state+=2;
            fgSplashProgress("loading-aircraft-list");
        }

    } else if ( idle_state == 2 ) {
        idle_state++;
        fgSplashProgress("loading-nav-dat");

    } else if ( idle_state == 3 ) {
        
        bool done = fgInitNav();
        if (done) {
          ++idle_state;
          fgSplashProgress("init-scenery");
        } else {
          fgSplashProgress("loading-nav-dat");
        }
      
    } else if ( idle_state == 4 ) {
        idle_state+=2;
        // based on the requested presets, calculate the true starting
        // lon, lat
        flightgear::initPosition();
        flightgear::initTowerLocationListener();

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

        flightgear::registerSubsystemCommands(globals->get_commands());

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

        fgSplashProgress("loading-aircraft");

    } else if ( idle_state == 6 ) {
        idle_state++;
        fgSplashProgress("creating-subsystems");

    } else if ( idle_state == 7 ) {
        idle_state++;
        SGTimeStamp st;
        st.stamp();
        fgCreateSubsystems();
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
                flightgear::setPosFromAirportIDandHdg( apt, hdg );
            }
        } else {
            SG_LOG(SG_ENVIRONMENT, SG_INFO,
                "No METAR available to pick active runway" );
        }

        fgSplashProgress("init-graphics");

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
        // We've finished all our initialization steps, from now on we
        // run the main loop.
        fgSetBool("sim/sceneryloaded", false);
        // stash current frame signal property
        frame_signal = fgGetNode("/sim/signals/frame", true);
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
    
    // delete the NavCache here. This will cause the destruction of many cached
    // objects (eg, airports, navaids, runways).
    delete flightgear::NavDataCache::instance();
  
    return result;
}
