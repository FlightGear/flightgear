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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#  include <float.h>
#endif

#include <plib/netSocket.h>

#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/sg_random.h>

// Class references
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/sky/sky.hxx>
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
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>
#include <FDM/flight.hxx>
// #include <FDM/ADA.hxx>
#include <ATC/ATCdisplay.hxx>
#include <ATC/ATCmgr.hxx>
#include <ATC/AIMgr.hxx>
#include <Time/tmp.hxx>
#include <Time/fg_timer.hxx>
#include <Environment/environment_mgr.hxx>
#include <GUI/new_gui.hxx>
#include <MultiPlayer/multiplaymgr.hxx>

#include "fg_commands.hxx"
#include "fg_io.hxx"
#include "renderer.hxx"
#include "splash.hxx"
#include "main.hxx"



static double real_delta_time_sec = 0.0;
double delta_time_sec = 0.0;
extern float init_volume;


#ifdef macintosh
#  include <console.h>		// -dw- for command line dialog
#endif

// This is a record containing a bit of global housekeeping information
FGGeneral general;

// Specify our current idle function state.  This is used to run all
// our initializations out of the idle callback so that we can get a
// splash screen up and running right away.
int idle_state = 0;
long global_multi_loop;

SGTimeStamp last_time_stamp;
SGTimeStamp current_time_stamp;

// The atexit() functio handler should know when the graphical subsystem
// is initialized.
extern int _bootstrap_OSInit;



// Update internal time dependent calculations (i.e. flight model)
// FIXME: this distinction is obsolete; all subsystems now get delta
// time on update.
void fgUpdateTimeDepCalcs() {
    static bool inited = false;

    static const SGPropertyNode *replay_state
        = fgGetNode( "/sim/freeze/replay-state", true );
    static SGPropertyNode *replay_time
        = fgGetNode( "/sim/replay/time", true );
    // static const SGPropertyNode *replay_end_time
    //     = fgGetNode( "/sim/replay/end-time", true );

    //SG_LOG(SG_FLIGHT,SG_INFO, "Updating time dep calcs()");

    // Initialize the FDM here if it hasn't been and if we have a
    // scenery elevation hit.

    // cout << "cur_fdm_state->get_inited() = " << cur_fdm_state->get_inited() 
    //      << " cur_elev = " << scenery.get_cur_elev() << endl;

    if (!cur_fdm_state->get_inited()) {
      // Check for scenery around the aircraft.
      double lon = fgGetDouble("/sim/presets/longitude-deg");
      double lat = fgGetDouble("/sim/presets/latitude-deg");
      // We require just to have 50 meter scenery availabe around
      // the aircraft.
      double range = 50.0;
      if (globals->get_tile_mgr()->scenery_available(lat, lon, range)) {
        SG_LOG(SG_FLIGHT,SG_INFO, "Finally initializing fdm");
        cur_fdm_state->init();
        if ( cur_fdm_state->get_bound() ) {
            cur_fdm_state->unbind();
        }
        cur_fdm_state->bind();
      }
    }

    // conceptually, the following block could be done for each fdm
    // instance ...
    if ( cur_fdm_state->get_inited() ) {
        // we have been inited, and  we are good to go ...

        if ( !inited ) {
            inited = true;
        }

        if ( replay_state->getIntValue() == 0 ) {
	    // replay off, run fdm
            cur_fdm_state->update( delta_time_sec );
        } else {
            FGReplay *r = (FGReplay *)(globals->get_subsystem( "replay" ));
            r->replay( replay_time->getDoubleValue() );
  	    if ( replay_state->getIntValue() == 1 ) {
                // normal playback
                replay_time->setDoubleValue( replay_time->getDoubleValue()
                                             + ( delta_time_sec
                                               * fgGetInt("/sim/speed-up") ) );
	    } else if ( replay_state->getIntValue() == 2 ) {
	        // paused playback (don't advance replay time)
	    }
	}
    } else {
        // do nothing, fdm isn't inited yet
    }

    globals->get_model_mgr()->update(delta_time_sec);
    globals->get_aircraft_model()->update(delta_time_sec);

    // update the view angle
    globals->get_viewmgr()->update(delta_time_sec);

    // Update solar system
    globals->get_ephem()->update( globals->get_time_params()->getMjd(),
                                  globals->get_time_params()->getLst(),
                                  cur_fdm_state->get_Latitude() );

}


void fgInitTimeDepCalcs( void ) {
    // noop for now
}


static const double alt_adjust_ft = 3.758099;
static const double alt_adjust_m = alt_adjust_ft * SG_FEET_TO_METER;


// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void ) {
    int model_hz = fgGetInt("/sim/model-hz");

    static const SGPropertyNode *longitude
        = fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
        = fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *altitude
        = fgGetNode("/position/altitude-ft");
    static const SGPropertyNode *clock_freeze
        = fgGetNode("/sim/freeze/clock", true);
    static const SGPropertyNode *cur_time_override
        = fgGetNode("/sim/time/cur-time-override", true);
    static const SGPropertyNode *max_simtime_per_frame
        = fgGetNode("/sim/max-simtime-per-frame", true);

    SGCloudLayer::enable_bump_mapping = fgGetBool("/sim/rendering/bump-mapping");

    bool scenery_loaded = fgGetBool("sim/sceneryloaded") || fgGetBool("sim/sceneryloaded-override");

    // Update the elapsed time.
    static bool first_time = true;
    if ( first_time ) {
        last_time_stamp.stamp();
        first_time = false;
    }

    double throttle_hz = fgGetDouble("/sim/frame-rate-throttle-hz", 0.0);
    if ( throttle_hz > 0.0 && scenery_loaded ) {
        // optionally throttle the frame rate (to get consistant frame
        // rates or reduce cpu usage.

        double frame_us = 1000000.0 / throttle_hz;

#define FG_SLEEP_BASED_TIMING 1
#if defined(FG_SLEEP_BASED_TIMING)
        // sleep based timing loop.
        //
        // Calling sleep, even usleep() on linux is less accurate than
        // we like, but it does free up the cpu for other tasks during
        // the sleep so it is desireable.  Because of the way sleep()
        // is implimented in consumer operating systems like windows
        // and linux, you almost always sleep a little longer than the
        // requested amount.
        // 
        // To combat the problem of sleeping to long, we calculate the
        // desired wait time and shorten it by 2000us (2ms) to avoid
        // [hopefully] over-sleep'ing.  The 2ms value was arrived at
        // via experimentation.  We follow this up at the end with a
        // simple busy-wait loop to get the final pause timing exactly
        // right.
        // 
        // Assuming we don't oversleep by more than 2000us, this
        // should be a reasonable compromise between sleep based
        // waiting, and busy waiting.

        // sleep() will always overshoot by a bit so undersleep by
        // 2000us in the hopes of never oversleeping.
        frame_us -= 2000.0;
        if ( frame_us < 0.0 ) {
            frame_us = 0.0;
        }
        current_time_stamp.stamp();
        /* Convert to ms */
        double elapsed_us = current_time_stamp - last_time_stamp;
        if ( elapsed_us < frame_us ) {
            double requested_us = frame_us - elapsed_us;
            ulMilliSecondSleep ( (int)(requested_us / 1000.0) ) ;
        }
#endif

        // busy wait timing loop.
        // 
        // This yields the most accurate timing.  If the previous
        // ulMilliSecondSleep() call is ommitted this will peg the cpu
        // (which is just fine if FG is the only app you care about.)
        current_time_stamp.stamp();
        while ( current_time_stamp - last_time_stamp < frame_us ) {
            current_time_stamp.stamp();
        }
    } else {
        // run as fast as the app will go
        current_time_stamp.stamp();
    }

    real_delta_time_sec
        = double(current_time_stamp - last_time_stamp) / 1000000.0;

    // Limit the time we need to spend in simulation loops
    // That means, if the /sim/max-simtime-per-frame value is strictly positive
    // you can limit the maximum amount of time you will do simulations for
    // one frame to display. The cpu time spent in simulations code is roughtly
    // at least O(real_delta_time_sec). If this is (due to running debug
    // builds or valgrind or something different blowing up execution times)
    // larger than the real time you will no more get any response
    // from flightgear. This limits that effect. Just set to property from
    // your .fgfsrc or commandline ...
    double dtMax = max_simtime_per_frame->getDoubleValue();
    if (0 < dtMax && dtMax < real_delta_time_sec)
      real_delta_time_sec = dtMax;

    // round the real time down to a multiple of 1/model-hz.
    // this way all systems are updated the _same_ amount of dt.
    {
      static double rem = 0.0;
      real_delta_time_sec += rem;
      double hz = model_hz;
      double nit = floor(real_delta_time_sec*hz);
      rem = real_delta_time_sec - nit/hz;
      real_delta_time_sec = nit/hz;
    }


    if (clock_freeze->getBoolValue() || !scenery_loaded) {
        delta_time_sec = 0;
    } else {
        delta_time_sec = real_delta_time_sec;
    }
    last_time_stamp = current_time_stamp;
    globals->inc_sim_time_sec( delta_time_sec );
    SGAnimation::set_sim_time_sec( globals->get_sim_time_sec() );

    // These are useful, especially for Nasal scripts.
    fgSetDouble("/sim/time/delta-realtime-sec", real_delta_time_sec);
    fgSetDouble("/sim/time/delta-sec", delta_time_sec);

    static long remainder = 0;
    long elapsed;
#ifdef FANCY_FRAME_COUNTER
    int i;
    double accum;
#else
    static time_t last_time = 0;
    static int frames = 0;
#endif // FANCY_FRAME_COUNTER

    SGTime *t = globals->get_time_params();

    globals->get_event_mgr()->update(delta_time_sec);

    SG_LOG( SG_ALL, SG_DEBUG, "Running Main Loop");
    SG_LOG( SG_ALL, SG_DEBUG, "======= ==== ====");

#if defined( ENABLE_PLIB_JOYSTICK )
    // Read joystick and update control settings
    // if ( fgGetString("/sim/control-mode") == "joystick" )
    // {
    //    fgJoystickRead();
    // }
#endif

    // Fix elevation.  I'm just sticking this here for now, it should
    // probably move eventually

    /* printf("Before - ground = %.2f  runway = %.2f  alt = %.2f\n",
           scenery.get_cur_elev(),
           cur_fdm_state->get_Runway_altitude() * SG_FEET_TO_METER,
           cur_fdm_state->get_Altitude() * SG_FEET_TO_METER); */

    /* printf("Adjustment - ground = %.2f  runway = %.2f  alt = %.2f\n",
           scenery.get_cur_elev(),
           cur_fdm_state->get_Runway_altitude() * SG_FEET_TO_METER,
           cur_fdm_state->get_Altitude() * SG_FEET_TO_METER); */

    // cout << "Warp = " << globals->get_warp() << endl;

    // update "time"
    static bool last_clock_freeze = false;

    if ( clock_freeze->getBoolValue() ) {
        // clock freeze requested
        if ( cur_time_override->getLongValue() == 0 ) {
            fgSetLong( "/sim/time/cur-time-override", t->get_cur_time() );
            globals->set_warp( 0 );
        }
    } else {
        // no clock freeze requested
        if ( last_clock_freeze == true ) {
            // clock just unfroze, let's set warp as the difference
            // between frozen time and current time so we don't get a
            // time jump (and corresponding sky object and lighting
            // jump.)
            globals->set_warp( cur_time_override->getLongValue() - time(NULL) );
            fgSetLong( "/sim/time/cur-time-override", 0 );
        }
        if ( globals->get_warp_delta() != 0 ) {
            globals->inc_warp( globals->get_warp_delta() );
        }
    }

    last_clock_freeze = clock_freeze->getBoolValue();

    t->update( longitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS,
               latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS,
               cur_time_override->getLongValue(),
               globals->get_warp() );

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

    // Get elapsed time (in usec) for this past frame
    elapsed = fgGetTimeInterval();
    SG_LOG( SG_ALL, SG_DEBUG, 
            "Elapsed time interval is = " << elapsed 
            << ", previous remainder is = " << remainder );

    // Calculate frame rate average
#ifdef FANCY_FRAME_COUNTER
    /* old fps calculation */
    if ( elapsed > 0 ) {
        double tmp;
        accum = 0.0;
        for ( i = FG_FRAME_RATE_HISTORY - 2; i >= 0; i-- ) {
            tmp = general.get_frame(i);
            accum += tmp;
            // printf("frame[%d] = %.2f\n", i, g->frames[i]);
            general.set_frame(i+1,tmp);
        }
        tmp = 1000000.0 / (float)elapsed;
        general.set_frame(0,tmp);
        // printf("frame[0] = %.2f\n", general.frames[0]);
        accum += tmp;
        general.set_frame_rate(accum / (float)FG_FRAME_RATE_HISTORY);
        // printf("ave = %.2f\n", general.frame_rate);
    }
#else
    if ( (t->get_cur_time() != last_time) && (last_time > 0) ) {
        general.set_frame_rate( frames );
        fgSetInt("/sim/frame-rate", frames);
        SG_LOG( SG_ALL, SG_DEBUG, 
            "--> Frame rate is = " << general.get_frame_rate() );
        frames = 0;
    }
    last_time = t->get_cur_time();
    ++frames;
#endif

    // Update any multiplayer's network queues, the AIMultiplayer
    // implementation is an AI model and depends on that
    globals->get_multiplayer_mgr()->Update();

    // Run ATC subsystem
    if (fgGetBool("/sim/atc/enabled"))
        globals->get_ATC_mgr()->update(delta_time_sec);

    // Run the AI subsystem
    // FIXME: run that also if we have multiplying enabled since the
    // multiplayer information is interpreted by an AI model
    if (fgGetBool("/sim/ai-traffic/enabled"))
        globals->get_AI_mgr()->update(delta_time_sec);

    // Run flight model

    // Calculate model iterations needed for next frame
    elapsed += remainder;

    global_multi_loop = (long)(((double)elapsed * 0.000001) * model_hz );
    remainder = elapsed - ( (global_multi_loop*1000000) / model_hz );
    SG_LOG( SG_ALL, SG_DEBUG, 
            "Model iterations needed = " << global_multi_loop
            << ", new remainder = " << remainder );
        
    // chop max interations to something reasonable if the sim was
    // delayed for an excesive amount of time
    if ( global_multi_loop > 2.0 * model_hz ) {
        global_multi_loop = (int)(2.0 * model_hz );
        remainder = 0;
    }

    // flight model
    if ( global_multi_loop > 0) {
        // first run the flight model each frame until it is intialized
        // then continue running each frame only after initial scenery load is complete.
        fgUpdateTimeDepCalcs();
    } else {
        SG_LOG( SG_ALL, SG_DEBUG, 
            "Elapsed time is zero ... we're zinging" );
    }

    // Do any I/O channel work that might need to be done
    globals->get_io()->update( real_delta_time_sec );

    // see if we need to load any deferred-load textures
    globals->get_matlib()->load_next_deferred();

    // Run audio scheduler
#ifdef ENABLE_AUDIO_SUPPORT
    if ( globals->get_soundmgr()->is_working() ) {
        globals->get_soundmgr()->update( delta_time_sec );
    }
#endif

    globals->get_subsystem_mgr()->update(delta_time_sec);

    //
    // Tile Manager updates - see if we need to load any new scenery tiles.
    //   this code ties together the fdm, viewer and scenery classes...
    //   we may want to move this to it's own class at some point
    //
    double visibility_meters = fgGetDouble("/environment/visibility-m");
    FGViewer *current_view = globals->get_current_view();

    // Let the scenery center follow the current view position with
    // 30m increments.
    //
    // Having the scenery center near the view position will eliminate
    // jitter of objects which are placed very near the view position
    // and haveing it's center near that view position.
    // So the 3d insruments of the aircraft will not jitter with this.
    // 
    // Following the view position exactly would introduce jitter of
    // the scenery tiles (they would be from their center up to 10000m
    // to the view and this will introduce roundoff too). By stepping
    // at 30m incements the roundoff error of the scenery tiles is
    // still present, but we will make exactly the same roundoff error
    // at each frame until the center is switched to a new
    // position. This roundoff is still visible but you will most
    // propably not notice.
    double *vp = globals->get_current_view()->get_absolute_view_pos();
    Point3D cntr(vp[0], vp[1], vp[2]);
    if (30.0*30.0 < cntr.distance3Dsquared(globals->get_scenery()->get_center())) {
      globals->get_scenery()->set_next_center( cntr );
    }

    globals->get_tile_mgr()->prep_ssg_nodes( current_view->getSGLocation(),
                                             visibility_meters );
    // update tile manager for view...
    SGLocation *view_location = globals->get_current_view()->getSGLocation();
    globals->get_tile_mgr()->update( view_location, visibility_meters );
    globals->get_scenery()->set_center( cntr );
    {
      double lon = view_location->getLongitude_deg();
      double lat = view_location->getLatitude_deg();
      double alt = view_location->getAltitudeASL_ft() * SG_FEET_TO_METER;

      // check if we can reuse the groundcache for that purpose.
      double ref_time, pt[3], r;
      bool valid = cur_fdm_state->is_valid_m(&ref_time, pt, &r);
      if (valid &&
          cntr.distance3Dsquared(Point3D(pt[0], pt[1], pt[2])) < r*r) {
        // Reuse the cache ...
        double lev
          = cur_fdm_state->get_groundlevel_m(lat*SGD_DEGREES_TO_RADIANS,
                                             lon*SGD_DEGREES_TO_RADIANS,
                                             alt + 2.0);
        view_location->set_cur_elev_m( lev );
      } else {
        // Do full intersection test.
        double lev;
        if (globals->get_scenery()->get_elevation_m(lat, lon, alt+2, lev, 0))
          view_location->set_cur_elev_m( lev );
        else
          view_location->set_cur_elev_m( -9999.0 );
      }
    }

#ifdef ENABLE_AUDIO_SUPPORT
    // Right now we make a simplifying assumption that the primary
    // aircraft is the source of all sounds and that all sounds are
    // positioned relative to the current view position.

    static sgVec3 last_pos_offset;

    // get the location data for the primary FDM (now hardcoded to ac model)...
    SGLocation *acmodel_loc = NULL;
    acmodel_loc = (SGLocation *)globals->
        get_aircraft_model()->get3DModel()->getSGLocation();

    // set positional offset for sources
    sgdVec3 dsource_pos_offset;
    sgdSubVec3( dsource_pos_offset,
                view_location->get_absolute_view_pos(),
                acmodel_loc->get_absolute_view_pos() );
    // cout << "pos all = " << source_pos_offset[0] << " " << source_pos_offset[1] << " " << source_pos_offset[2] << endl;
    sgVec3 source_pos_offset;
    sgSetVec3(source_pos_offset, dsource_pos_offset);
    globals->get_soundmgr()->set_source_pos_all( source_pos_offset );

    // set the velocity
    sgVec3 source_vel;
    sgSubVec3( source_vel, source_pos_offset, last_pos_offset );
    sgScaleVec3( source_vel, delta_time_sec );
    sgCopyVec3( last_pos_offset, source_pos_offset );
    // cout << "vel = " << source_vel[0] << " " << source_vel[1] << " " << source_vel[2] << endl;
    globals->get_soundmgr()->set_source_vel_all( source_vel );

    // Right now we make a simplifying assumption that the listener is
    // always positioned at the origin.
    sgVec3 listener_pos;
    sgSetVec3( listener_pos, 0.0, 0.0, 0.0 );
    // cout << "listener = " << listener_pos[0] << " " << listener_pos[1] << " " << listener_pos[2] << endl;
    globals->get_soundmgr()->set_listener_pos( listener_pos );
#endif

    // END Tile Manager udpates

    if (!scenery_loaded && globals->get_tile_mgr()->all_queues_empty() && cur_fdm_state->get_inited()) {
        fgSetBool("sim/sceneryloaded",true);
        fgSetFloat("/sim/sound/volume", init_volume);
        globals->get_soundmgr()->set_volume(init_volume);
    }

    if (fgGetBool("/sim/rendering/specular-highlight")) {
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	// glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    } else {
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
        // glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    }

    fgRequestRedraw();

    SG_LOG( SG_ALL, SG_DEBUG, "" );
}


// This is the top level master main function that is registered as
// our idle funciton

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static void fgIdleFunction ( void ) {
    if ( idle_state == 0 ) {
        idle_state++;
        fgSplashProgress("setting up scenegraph & user interface");


    } else if ( idle_state == 1 ) {
        idle_state++;
        // This seems to be the absolute earliest in the init sequence
        // that these calls will return valid info.  Too bad it's after
        // we've already created and sized out window. :-(
        general.set_glVendor( (char *)glGetString ( GL_VENDOR ) );
        general.set_glRenderer( (char *)glGetString ( GL_RENDERER ) );
        general.set_glVersion( (char *)glGetString ( GL_VERSION ) );
        SG_LOG( SG_GENERAL, SG_INFO, general.get_glRenderer() );

        GLint tmp;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &tmp );
        general.set_glMaxTexSize( tmp );
        SG_LOG ( SG_GENERAL, SG_INFO, "Max texture size = " << tmp );

        glGetIntegerv( GL_DEPTH_BITS, &tmp );
        general.set_glDepthBits( tmp );
        SG_LOG ( SG_GENERAL, SG_INFO, "Depth buffer bits = " << tmp );

        // Initialize ssg (from plib).  Needs to come before we do any
        // other ssg stuff, but after opengl has been initialized.
        ssgInit();

        // Initialize the user interface (we need to do this before
        // passing off control to the OS main loop and before
         // fgInitGeneral to get our fonts !!!
        guiInit();
        fgSplashProgress("reading aircraft list");


    } else if ( idle_state == 2 ) {
        idle_state++;
        // Read the list of available aircrafts
        fgReadAircraft();

#ifdef GL_EXT_texture_lod_bias
        glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, -0.5 ) ;
#endif

        // get the address of our OpenGL extensions
        if ( fgGetBool("/sim/rendering/distance-attenuation") ) {
            if (SGIsOpenGLExtensionSupported("GL_EXT_point_parameters") ) {
                glPointParameterIsSupported = true;
                glPointParameterfPtr = (glPointParameterfProc)
                                       SGLookupFunction("glPointParameterfEXT");
                glPointParameterfvPtr = (glPointParameterfvProc)
                                        SGLookupFunction("glPointParameterfvEXT");

            } else if ( SGIsOpenGLExtensionSupported("GL_ARB_point_parameters") ) {
                glPointParameterIsSupported = true;
                glPointParameterfPtr = (glPointParameterfProc)
                                       SGLookupFunction("glPointParameterfARB");
                glPointParameterfvPtr = (glPointParameterfvProc)
                                        SGLookupFunction("glPointParameterfvARB");
            } else
                glPointParameterIsSupported = false;
        }
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

        SGTime *t = fgInitTime();
        globals->set_time_params( t );

        // Do some quick general initializations
        if( !fgInitGeneral()) {
            SG_LOG( SG_GENERAL, SG_ALERT, 
                "General initializations failed ..." );
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
        globals->set_model_lib(new SGModelLib);


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
        globals->set_aircraft_model(new FGAircraftModel);
        globals->get_aircraft_model()->init();
        globals->get_aircraft_model()->bind();

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
        SGPath ephem_data_path( globals->get_fg_root() );
        ephem_data_path.append( "Astro" );
        SGEphemeris *ephem = new SGEphemeris( ephem_data_path.c_str() );
        ephem->update( globals->get_time_params()->getMjd(),
                       globals->get_time_params()->getLst(),
                       0.0 );
        globals->set_ephem( ephem );

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
        // actual diameters. This was needed to fit bot the sun and the
        // moon within the distance to the far clip plane.
        // Moon diameter:    3,476 kilometers
        // Sun diameter: 1,390,000 kilometers
        thesky->build( 80000.0, 80000.0,
                       463.3, 361.8,
                       globals->get_ephem()->getNumPlanets(), 
                       globals->get_ephem()->getPlanets(),
                       globals->get_ephem()->getNumStars(),
                       globals->get_ephem()->getStars() );

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

        // airport = new ssgBranch;
        // airport->setName( "Airport Lighting" );
        // lighting->addKid( airport );

        // build our custom render states
        globals->get_renderer()->build_states();
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

#if defined( __CYGWIN__ )
            string command = "start /m `cygpath -w " + mp3file.str() + "`";
#elif defined( WIN32 )
            string command = "start /m " + mp3file.str();
#else
            string command = "mpg123 " + mp3file.str() + "> /dev/null 2>&1";
#endif

            system ( command.c_str() );
        }
#endif

        // These are a few miscellaneous things that aren't really
        // "subsystems" but still need to be initialized.

#ifdef USE_GLIDE
        if ( strstr ( general.get_glRenderer(), "Glide" ) ) {
            grTexLodBiasValue ( GR_TMU0, 1.0 ) ;
        }
#endif

        // This is the top level init routine which calls all the
        // other subsystem initialization routines.  If you are adding
        // a subsystem to flightgear, its initialization call should
        // located in this routine.
        if( !fgInitSubsystems()) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                "Subsystem initializations failed ..." );
            exit(-1);
        }
        fgSplashProgress("setting up time & renderer");


    } else if ( idle_state == 8 ) {
        idle_state = 1000;
        // Initialize the time offset (warp) after fgInitSubsystem
        // (which initializes the lighting interpolation tables.)
        fgInitTimeOffset();

        // setup OpenGL view parameters
        globals->get_renderer()->init();

        SG_LOG( SG_GENERAL, SG_INFO, "Panel visible = " << fgPanelVisible() );
        globals->get_renderer()->resize( fgGetInt("/sim/startup/xsize"),
                                         fgGetInt("/sim/startup/ysize") );

        fgSplashProgress("loading scenery objects");

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
    SGPropertyNode *p = fgGetNode(name, false);
    if (!p) {
        p = fgGetNode(name, true);
        p->setStringValue("");
    } else {
        SGPropertyNode::Type t = p->getType();
        if (t == SGPropertyNode::NONE || t == SGPropertyNode::UNSPECIFIED)
            p->setStringValue("");
        else
            assert(t == SGPropertyNode::STRING);
    }
    p->addChangeListener(new FGMakeUpperCase);
}


// Main top level initialization
bool fgMainInit( int argc, char **argv ) {

#if defined( macintosh )
    freopen ("stdout.txt", "w", stdout );
    freopen ("stderr.txt", "w", stderr );
    argc = ccommand( &argv );
#endif

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

    // seed the random number generater
    sg_srandom_time();

    FGControls *controls = new FGControls;
    globals->set_controls( controls );

    string_list *col = new string_list;
    globals->set_channel_options_list( col );

    upper_case_property("/sim/presets/airport-id");
    upper_case_property("/sim/presets/runway");
    upper_case_property("/sim/tower/airport-id");

    // Scan the config file(s) and command line options to see if
    // fg_root was specified (ignore all other options for now)
    fgInitFGRoot(argc, argv);

    // Check for the correct base package version
    static char required_version[] = "0.9.10";
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

    sgUseDisplayList = fgGetBool( "/sim/rendering/use-display-list", true );

    // Load the configuration parameters.  (Command line options
    // overrides config file options.  Config file options override
    // defaults.)
    if ( !fgInitConfig(argc, argv) ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Config option parsing failed ..." );
        exit(-1);
    }

    // Initialize the Window/Graphics environment.
#if !defined(__APPLE__) || defined(OSX_BUNDLE)
    // Mac OS X command line ("non-bundle") applications call this
    // from main(), in bootstrap.cxx.  Andy doesn't know why, someone
    // feel free to add comments...
    fgOSInit(&argc, argv);
    _bootstrap_OSInit++;
#endif

    fgRegisterWindowResizeHandler( &FGRenderer::resize );
    fgRegisterIdleHandler( &fgIdleFunction );
    fgRegisterDrawHandler( &FGRenderer::update );

#ifdef FG_ENABLE_MULTIPASS_CLOUDS
    bool get_stencil_buffer = true;
#else
    bool get_stencil_buffer = false;
#endif

    // Initialize plib net interface
    netInit( &argc, argv );

    // Clouds3D requires an alpha channel
    // clouds may require stencil buffer
    fgOSOpenWindow( fgGetInt("/sim/startup/xsize"),
                    fgGetInt("/sim/startup/ysize"),
                    fgGetInt("/sim/rendering/bits-per-pixel"),
                    fgGetBool("/sim/rendering/clouds3d-enable"),
                    get_stencil_buffer,
                    fgGetBool("/sim/startup/fullscreen") );

    // Initialize the splash screen right away
    fntInit();
    fgSplashInit(fgGetString("/sim/startup/splash-texture"));

    // pass control off to the master event handler
    fgOSMainLoop();

    // we never actually get here ... but to avoid compiler warnings,
    // etc.
    return false;
}


