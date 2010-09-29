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
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/sg_random.h>

#include <Time/light.hxx>
#include <Include/general.hxx>
#include <Aircraft/replay.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/hud.hxx>
#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Sound/fg_fx.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>
#include <Sound/fg_fx.hxx>
#include <ATCDCL/ATCmgr.hxx>
#include <Time/TimeManager.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/ephemeris.hxx>
#include <GUI/new_gui.hxx>
#include <MultiPlayer/multiplaymgr.hxx>

#include "CameraGroup.hxx"
#include "fg_commands.hxx"
#include "fg_io.hxx"
#include "renderer.hxx"
#include "splash.hxx"
#include "main.hxx"
#include "util.hxx"
#include "fg_init.hxx"
#include "fg_os.hxx"
#include "WindowSystemAdapter.hxx"
#include <Main/viewer.hxx>


using namespace flightgear;

using std::cerr;

// This is a record containing a bit of global housekeeping information
FGGeneral general;

// Specify our current idle function state.  This is used to run all
// our initializations out of the idle callback so that we can get a
// splash screen up and running right away.
int idle_state = 0;


void fgInitSoundManager();
void fgSetNewSoundDevice(const char *);

// The atexit() function handler should know when the graphical subsystem
// is initialized.
extern int _bootstrap_OSInit;

// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void ) {
    
    static SGConstPropertyNode_ptr longitude
        = fgGetNode("/position/longitude-deg");
    static SGConstPropertyNode_ptr latitude
        = fgGetNode("/position/latitude-deg");
    static SGConstPropertyNode_ptr altitude
        = fgGetNode("/position/altitude-ft");
    static SGConstPropertyNode_ptr vn_fps
        = fgGetNode("/velocities/speed-north-fps");
    static SGConstPropertyNode_ptr ve_fps
        = fgGetNode("/velocities/speed-east-fps");
    static SGConstPropertyNode_ptr vd_fps
        = fgGetNode("/velocities/speed-down-fps");
      
    static SGPropertyNode_ptr frame_signal
        = fgGetNode("/sim/signals/frame", true);

    frame_signal->fireValueChanged();
    SGCloudLayer::enable_bump_mapping = fgGetBool("/sim/rendering/bump-mapping");
    
    SG_LOG( SG_ALL, SG_DEBUG, "Running Main Loop");
    SG_LOG( SG_ALL, SG_DEBUG, "======= ==== ====");
    
    
  // update "time"
    double sim_dt, real_dt;
    TimeManager* timeMgr = (TimeManager*) globals->get_subsystem("time");
    // compute simulated time (allowing for pause, warp, etc) and
    // real elapsed time
    timeMgr->computeTimeDeltas(sim_dt, real_dt);
    
    if (globals->get_warp_delta() != 0) {
        FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));
        l->update( 0.5 );
    }

    // update magvar model
    globals->get_mag()->update( longitude->getDoubleValue()
                                * SGD_DEGREES_TO_RADIANS,
                                latitude->getDoubleValue()
                                * SGD_DEGREES_TO_RADIANS,
                                altitude->getDoubleValue() * SG_FEET_TO_METER,
                                globals->get_time_params()->getJD() );


    // Update any multiplayer's network queues, the AIMultiplayer
    // implementation is an AI model and depends on that
    globals->get_multiplayer_mgr()->Update();

#if ENABLE_ATCDCL  
    // Run ATC subsystem
    if (fgGetBool("/sim/atc/enabled"))
        globals->get_ATC_mgr()->update(sim_dt);
#endif  
    
    globals->get_subsystem_mgr()->update(sim_dt);
    globals->get_aircraft_model()->update(sim_dt);
    
    //
    // Tile Manager updates - see if we need to load any new scenery tiles.
    //   this code ties together the fdm, viewer and scenery classes...
    //   we may want to move this to its own class at some point
    //
    double visibility_meters = fgGetDouble("/environment/visibility-m");
    globals->get_tile_mgr()->prep_ssg_nodes( visibility_meters );

    // update tile manager for view...
    SGVec3d viewPos = globals->get_current_view()->get_view_pos();
    SGGeod geodViewPos = SGGeod::fromCart(viewPos);
    globals->get_tile_mgr()->update(geodViewPos, visibility_meters);

    // run Nasal's settimer() loops right before the view manager
    globals->get_event_mgr()->update(sim_dt);

    // pick up model coordidnates that Nasal code may have set relative to the
    // aircraft's
    globals->get_model_mgr()->update(sim_dt);

    // update the view angle as late as possible, but before sound calculations
    globals->get_viewmgr()->update(real_dt);

    // Update the sound manager last so it can use the CPU while the GPU
    // is processing the scenery (doubled the frame-rate for me) -EMH-
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

        if (sound_working->getBoolValue() == false) {	// request to reinit
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
            smgr->update(sim_dt);
        }
    }
#endif

    // END Tile Manager udpates
    bool scenery_loaded = fgGetBool("sim/sceneryloaded");
    if (!scenery_loaded && globals->get_tile_mgr()->isSceneryLoaded()
        && fgGetBool("sim/fdm-initialized")) {
        fgSetBool("sim/sceneryloaded",true);
        if (fgGetBool("/sim/sound/working")) {
            globals->get_soundmgr()->activate();
        }
        globals->get_props()->tie("/sim/sound/devices/name",
              SGRawValueFunctions<const char *>(0, fgSetNewSoundDevice), false);
    }
    simgear::AtomicChangeListener::fireChangeListeners();

    SG_LOG( SG_ALL, SG_DEBUG, "" );
}

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
    globals->get_soundmgr()->suspend();
    globals->get_soundmgr()->stop();
    globals->get_soundmgr()->init(device);
    globals->get_soundmgr()->resume();
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
        general.set_glVendor( (char *)glGetString ( GL_VENDOR ) );
        general.set_glRenderer( (char *)glGetString ( GL_RENDERER ) );
        general.set_glVersion( (char *)glGetString ( GL_VERSION ) );
        SG_LOG( SG_GENERAL, SG_INFO, general.get_glVendor() );
        SG_LOG( SG_GENERAL, SG_INFO, general.get_glRenderer() );
        SG_LOG( SG_GENERAL, SG_INFO, general.get_glVersion() );

        GLint tmp;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &tmp );
        general.set_glMaxTexSize( tmp );
        SG_LOG ( SG_GENERAL, SG_INFO, "Max texture size = " << tmp );

        glGetIntegerv( GL_DEPTH_BITS, &tmp );
        general.set_glDepthBits( tmp );
        SG_LOG ( SG_GENERAL, SG_INFO, "Depth buffer bits = " << tmp );
    }
};


osg::Node* load_panel(SGPropertyNode *n)
{
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(new FGPanelNode(n));
    return geode;
}

SGPath resolve_path(const std::string& s)
{
  return globals->resolve_maybe_aircraft_path(s);
}

}

// This is the top level master main function that is registered as
// our idle funciton

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
        fgSplashProgress("reading aircraft list");


    } else if ( idle_state == 2 ) {
        idle_state++;
                
        fgSplashProgress("reading airport & navigation data");


    } else if ( idle_state == 3 ) {
        idle_state++;
        fgInitNav();
        fgSplashProgress("setting up scenery");


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
        simgear::SGModelLib::init(globals->get_fg_root());
        simgear::SGModelLib::setPropRoot(globals->get_props());
        simgear::SGModelLib::setPanelFunc(load_panel);
        
        ////////////////////////////////////////////////////////////////////
        // Initialize the TG scenery subsystem.
        ////////////////////////////////////////////////////////////////////
        globals->set_scenery( new FGScenery );
        globals->get_scenery()->init();
        globals->get_scenery()->bind();
        globals->set_tile_mgr( new FGTileMgr );


        ////////////////////////////////////////////////////////////////////
        // Initialize the general model subsystem.
        ////////////////////////////////////////////////////////////////////
        globals->set_model_mgr(new FGModelMgr);
        globals->get_model_mgr()->init();
        globals->get_model_mgr()->bind();
        fgSplashProgress("loading aircraft");


    } else if ( idle_state == 5 ) {
        idle_state++;

        ////////////////////////////////////////////////////////////////////
        // Initialize the 3D aircraft model subsystem (has a dependency on
        // the scenery subsystem.)
        ////////////////////////////////////////////////////////////////////
        FGAircraftModel* acm = new FGAircraftModel;
        globals->set_aircraft_model(acm);
        //globals->add_subsystem("aircraft-model", acm);
        acm->init();
        acm->bind();

        ////////////////////////////////////////////////////////////////////
        // Initialize the view manager subsystem.
        ////////////////////////////////////////////////////////////////////
        FGViewMgr *viewmgr = new FGViewMgr;
        globals->set_viewmgr( viewmgr );
        viewmgr->init();
        viewmgr->bind();
        fgSplashProgress("generating sky elements");


    } else if ( idle_state == 6 ) {
        idle_state++;
        // Initialize the sky

        Ephemeris* eph = new Ephemeris;
        globals->add_subsystem("ephemeris", eph);
        eph->init(); // FIXME - remove this once SGSky code below is also a subsystem
        eph->bind();

        // TODO: move to environment mgr
        thesky = new SGSky;
        SGPath texture_path(globals->get_fg_root());
        texture_path.append("Textures");
        texture_path.append("Sky");
        for (int i = 0; i < FGEnvironmentMgr::MAX_CLOUD_LAYERS; i++) {
            SGCloudLayer * layer = new SGCloudLayer(texture_path.str());
            thesky->add_cloud_layer(layer);
        }

        SGPath sky_tex_path( globals->get_fg_root() );
        sky_tex_path.append( "Textures" );
        sky_tex_path.append( "Sky" );
        thesky->texture_path( sky_tex_path.str() );

        // The sun and moon diameters are scaled down numbers of the
        // actual diameters. This was needed to fit both the sun and the
        // moon within the distance to the far clip plane.
        // Moon diameter:    3,476 kilometers
        // Sun diameter: 1,390,000 kilometers
        thesky->build( 80000.0, 80000.0,
                       463.3, 361.8,
                       *globals->get_ephem(),
                       fgGetNode("/environment", true));

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
        double var = globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES;
        fgSetDouble("/instrumentation/heading-indicator/offset-deg", -var);
        fgSetDouble("/instrumentation/heading-indicator-fg/offset-deg", -var);


        // airport = new ssgBranch;
        // airport->setName( "Airport Lighting" );
        // lighting->addKid( airport );

        // build our custom render states
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

            system ( command.c_str() );
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
        fgSplashProgress("setting up time & renderer");


    } else if ( idle_state == 8 ) {
        idle_state = 1000;
        
        // setup OpenGL view parameters
        globals->get_renderer()->init();

        globals->get_renderer()->resize( fgGetInt("/sim/startup/xsize"),
                                         fgGetInt("/sim/startup/ysize") );

        fgSplashProgress("loading scenery objects");
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
    SG_LOG( SG_GENERAL, SG_INFO, "Built with " << SG_COMPILER_STR << endl );

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

    // Scan the config file(s) and command line options to see if
    // fg_root was specified (ignore all other options for now)
    fgInitFGRoot(argc, argv);

    // Check for the correct base package version
    static char required_version[] = "2.0.0";
    string base_version = fgBasePackageVersion();
    if ( !(base_version == required_version) ) {
        // tell the operator how to use this application

        SG_LOG( SG_GENERAL, SG_ALERT, "" ); // To popup the console on windows
        cerr << endl << "Base package check failed ... " \
             << "Found version " << base_version << " at: " \
             << globals->get_fg_root() << endl;
        cerr << "Please upgrade to version: " << required_version << endl;
#ifdef _MSC_VER
        cerr << "Hit a key to continue..." << endl;
        cin.get();
#endif
        exit(-1);
    }

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

    fgRegisterWindowResizeHandler( &FGRenderer::resize );
    fgRegisterIdleHandler( &fgIdleFunction );
    fgRegisterDrawHandler( &FGRenderer::update );

    // Initialize plib net interface
    netInit( &argc, argv );

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


