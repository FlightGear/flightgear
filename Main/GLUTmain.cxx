// GLUTmain.cxx -- top level sim routines
//
// Written by Curtis Olson for OpenGL, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)

#define MICHAEL_JOHNSON_EXPERIMENTAL_ENGINE_AUDIO

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#  include <float.h>                    
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef HAVE_STDLIB_H
#   include <stdlib.h>
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h> /* for stat() */
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>    /* for stat() */
#endif

#include <Include/fg_constants.h>  // for VERSION
#include <Include/general.hxx>

#include <Aircraft/aircraft.hxx>
#include <Astro/sky.hxx>
#include <Astro/stars.hxx>
#include <Astro/solarsystem.hxx>

#ifdef ENABLE_AUDIO_SUPPORT
#  include <Audio/src/sl.h>
#  include <Audio/src/sm.h>
#endif

#include <Autopilot/autopilot.hxx>
#include <Cockpit/cockpit.hxx>
#include <Debug/logstream.hxx>
#include <GUI/gui.h>
#include <Joystick/joystick.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <PUI/pu.h>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>
#include <Time/fg_timer.hxx>
#include <Time/sunpos.hxx>
#include <Weather/weather.hxx>

#include "GLUTkey.hxx"
#include "fg_init.hxx"
#include "options.hxx"
#include "splash.hxx"
#include "views.hxx"


// This is a record containing a bit of global housekeeping information
FGGeneral general;

// Specify our current idle function state.  This is used to run all
// our initializations out of the glutIdleLoop() so that we can get a
// splash screen up and running right away.
static int idle_state = 0;

// Another hack
int use_signals = 0;

// Yet another hack, this time for the panel
int panel_hist = 0;

// Global structures for the Audio library
#ifdef ENABLE_AUDIO_SUPPORT
slEnvelope pitch_envelope ( 1, SL_SAMPLE_ONE_SHOT ) ;
slEnvelope volume_envelope ( 1, SL_SAMPLE_ONE_SHOT ) ;
slScheduler *audio_sched;
smMixer *audio_mixer;
slSample *s1;
slSample *s2;
#endif


// The following defines flight gear options. Because glutlib will also
// want to parse its own options, those options must not be included here
// or they will get parsed by the main program option parser. Hence case
// is significant for any option added that might be in conflict with
// glutlib's parser.
//
// glutlib parses for:
//    -display
//    -direct   (invalid in Win32)
//    -geometry
//    -gldebug
//    -iconized
//    -indirect (invalid in Win32)
//    -synce
//
// Note that glutlib depends upon strings while this program's
// option parser wants only initial characters followed by numbers
// or pathnames.
//


// fgInitVisuals() -- Initialize various GL/view parameters
static void fgInitVisuals( void ) {
    fgLIGHT *l;

    l = &cur_light_params;

    // Go full screen if requested ...
    if ( current_options.get_fullscreen() ) {
	glutFullScreen();
    }

    // If enabled, normal vectors specified with glNormal are scaled
    // to unit length after transformation.  See glNormal.
    // xglEnable( GL_NORMALIZE );

    xglEnable( GL_LIGHTING );
    xglEnable( GL_LIGHT0 );
    xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

    // xglFogi (GL_FOG_MODE, GL_LINEAR);
    xglFogi (GL_FOG_MODE, GL_EXP2);
    // Fog density is now set when the weather system is initialized
    // xglFogf (GL_FOG_DENSITY, w->fog_density);
    if ( (current_options.get_fog() == 1) || 
	 (current_options.get_shading() == 0) ) {
	// if fastest fog requested, or if flat shading force fastest
	xglHint ( GL_FOG_HINT, GL_FASTEST );
    } else if ( current_options.get_fog() == 2 ) {
	xglHint ( GL_FOG_HINT, GL_NICEST );
    }
    if ( current_options.get_wireframe() ) {
	// draw wire frame
	xglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    // This is the default anyways, but it can't hurt
    xglFrontFace ( GL_CCW );

    // Just testing ...
    // xglEnable(GL_POINT_SMOOTH);
    // xglEnable(GL_LINE_SMOOTH);
    // xglEnable(GL_POLYGON_SMOOTH);      
}


#if 0
// Draw a basic instrument panel
static void fgUpdateInstrViewParams( void ) {

    exit(0);

    fgVIEW *v = &current_view;

    xglViewport(0, 0 , (GLint)(v->winWidth), (GLint)(v->winHeight) / 2);
  
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
  
    xglLoadIdentity();
    gluOrtho2D(0, 640, 0, 480);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();

    xglColor3f(1.0, 1.0, 1.0);
    xglIndexi(7);
  
    xglDisable(GL_DEPTH_TEST);
    xglDisable(GL_LIGHTING);
  
    xglLineWidth(1);
    xglColor3f (0.5, 0.5, 0.5);

    xglBegin(GL_QUADS);
    xglVertex2f(0.0, 0.00);
    xglVertex2f(0.0, 480.0);
    xglVertex2f(640.0,480.0);
    xglVertex2f(640.0, 0.0);
    xglEnd();

    xglRectf(0.0,0.0, 640, 480);
    xglEnable(GL_DEPTH_TEST);
    xglEnable(GL_LIGHTING);
    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}
#endif


// Update all Visuals (redraws anything graphics related)
static void fgRenderFrame( void ) {
    fgLIGHT *l = &cur_light_params;
    fgTIME *t = &cur_time_params;
    FGView *v = &current_view;

    double angle;
    static int iteration = 0;
    // GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat terrain_color[4] = { 0.54, 0.44, 0.29, 1.0 };
    GLbitfield clear_mask;

    if ( idle_state != 1000 ) {
	// still initializing, draw the splash screen
	if ( current_options.get_splash_screen() == 1 ) {
	    fgSplashUpdate(0.0);
	}
    } else {
	// idle_state is now 1000 meaning we've finished all our
	// initializations and are running the main loop, so this will
	// now work without seg faulting the system.

	// printf("Ground = %.2f  Altitude = %.2f\n", scenery.cur_elev, 
	//        FG_Altitude * FEET_TO_METER);
    
	// this is just a temporary hack, to make me understand Pui
	// timerText -> setLabel (ctime (&t->cur_time));
	// end of hack

	// update view volume parameters
	v->UpdateViewParams();

	// set the sun position
	xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

	clear_mask = GL_DEPTH_BUFFER_BIT;
	if ( current_options.get_wireframe() ) {
	    clear_mask |= GL_COLOR_BUFFER_BIT;
	}
	if ( current_options.get_panel_status() ) {
	    // we can't clear the screen when the panel is active
	} else if ( current_options.get_skyblend() ) {
	    if ( current_options.get_textures() ) {
		// glClearColor(black[0], black[1], black[2], black[3]);
		glClearColor(l->adj_fog_color[0], l->adj_fog_color[1], 
			     l->adj_fog_color[2], l->adj_fog_color[3]);
		clear_mask |= GL_COLOR_BUFFER_BIT;		
	    }
	} else {
	    glClearColor(l->sky_color[0], l->sky_color[1], 
			 l->sky_color[2], l->sky_color[3]);
	    clear_mask |= GL_COLOR_BUFFER_BIT;
	}
	xglClear( clear_mask );

	// Tell GL we are switching to model view parameters
	xglMatrixMode(GL_MODELVIEW);
	// xglLoadIdentity();

	// draw sky
	xglDisable( GL_DEPTH_TEST );
	xglDisable( GL_LIGHTING );
	xglDisable( GL_CULL_FACE );
	xglDisable( GL_FOG );
	xglShadeModel( GL_SMOOTH );
	if ( current_options.get_skyblend() ) {
	    fgSkyRender();
	}

	// setup transformation for drawing astronomical objects
	xglPushMatrix();
	// Translate to view position
	Point3D view_pos = v->get_view_pos();
	xglTranslatef( view_pos.x(), view_pos.y(), view_pos.z() );
	// Rotate based on gst (sidereal time)
	// note: constant should be 15.041085, Curt thought it was 15
	angle = t->gst * 15.041085;
	// printf("Rotating astro objects by %.2f degrees\n",angle);
	xglRotatef( angle, 0.0, 0.0, -1.0 );

	// draw stars and planets
	fgStarsRender();
	SolarSystem::theSolarSystem->draw();

	xglPopMatrix();

	// draw scenery
	if ( current_options.get_shading() ) {
	    xglShadeModel( GL_SMOOTH ); 
	} else {
	    xglShadeModel( GL_FLAT ); 
	}
	xglEnable( GL_DEPTH_TEST );
	if ( current_options.get_fog() > 0 ) {
	    xglEnable( GL_FOG );
	    xglFogfv (GL_FOG_COLOR, l->adj_fog_color);
	}
	// set lighting parameters
	xglLightfv(GL_LIGHT0, GL_AMBIENT, l->scene_ambient );
	xglLightfv(GL_LIGHT0, GL_DIFFUSE, l->scene_diffuse );
	
	if ( current_options.get_textures() ) {
	    // texture parameters
	    xglEnable( GL_TEXTURE_2D );
	    xglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
	    xglHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;
	    // set base color (I don't think this is doing anything here)
	    xglMaterialfv (GL_FRONT, GL_AMBIENT, white);
	    xglMaterialfv (GL_FRONT, GL_DIFFUSE, white);
	} else {
	    xglDisable( GL_TEXTURE_2D );
	    xglMaterialfv (GL_FRONT, GL_AMBIENT, terrain_color);
	    xglMaterialfv (GL_FRONT, GL_DIFFUSE, terrain_color);
	    // xglMaterialfv (GL_FRONT, GL_AMBIENT, white);
	    // xglMaterialfv (GL_FRONT, GL_DIFFUSE, white);
	}

	fgTileMgrRender();

	xglDisable( GL_TEXTURE_2D );
	xglDisable( GL_FOG );

	if ( (iteration == 0) && (current_options.get_panel_status()) ) {   
	    // Did we run this loop before ?? ...and do we need the panel ??
	    fgPanelReInit(0, 0, 1024, 768);
	}

	// display HUD && Panel
	fgCockpitUpdate();
	iteration = 1; // don't ReInit the panel in the future

	// We can do translucent menus, so why not. :-)
	xglEnable    ( GL_BLEND ) ;
	xglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	puDisplay();
	xglDisable   ( GL_BLEND ) ;
	xglEnable( GL_FOG );
    }

    xglutSwapBuffers();
}


// Update internal time dependent calculations (i.e. flight model)
void fgUpdateTimeDepCalcs(int multi_loop, int remainder) {
    FGState *f = current_aircraft.fdm_state;
    fgLIGHT *l = &cur_light_params;
    fgTIME *t = &cur_time_params;
    FGView *v = &current_view;
    int i;

    // update the flight model
    if ( multi_loop < 0 ) {
	multi_loop = DEFAULT_MULTILOOP;
    }

    if ( !t->pause ) {
	// run Autopilot system
	fgAPRun();

	// printf("updating flight model x %d\n", multi_loop);
	fgFlightModelUpdate( current_options.get_flight_model(), 
			     cur_fdm_state, multi_loop, remainder );
    } else {
	fgFlightModelUpdate( current_options.get_flight_model(), 
			     cur_fdm_state, 0, remainder );
    }

    // update the view angle
    for ( i = 0; i < multi_loop; i++ ) {
	if ( fabs(v->get_goal_view_offset() - v->get_view_offset()) < 0.05 ) {
	    v->set_view_offset( v->get_goal_view_offset() );
	    break;
	} else {
	    // move v->view_offset towards v->goal_view_offset
	    if ( v->get_goal_view_offset() > v->get_view_offset() ) {
		if ( v->get_goal_view_offset() - v->get_view_offset() < FG_PI ){
		    v->inc_view_offset( 0.01 );
		} else {
		    v->inc_view_offset( -0.01 );
		}
	    } else {
		if ( v->get_view_offset() - v->get_goal_view_offset() < FG_PI ){
		    v->inc_view_offset( -0.01 );
		} else {
		    v->inc_view_offset( 0.01 );
		}
	    }
	    if ( v->get_view_offset() > FG_2PI ) {
		v->inc_view_offset( -FG_2PI );
	    } else if ( v->get_view_offset() < 0 ) {
		v->inc_view_offset( FG_2PI );
	    }
	}
    }

    double tmp = -(l->sun_rotation + FG_PI) 
	- (f->get_Psi() - v->get_view_offset() );
    while ( tmp < 0.0 ) {
	tmp += FG_2PI;
    }
    while ( tmp > FG_2PI ) {
	tmp -= FG_2PI;
    }
    /* printf("Psi = %.2f, viewoffset = %.2f sunrot = %.2f rottosun = %.2f\n",
	   FG_Psi * RAD_TO_DEG, v->view_offset * RAD_TO_DEG, 
	   -(l->sun_rotation+FG_PI) * RAD_TO_DEG, tmp * RAD_TO_DEG); */
    l->UpdateAdjFog();
}


void fgInitTimeDepCalcs( void ) {
    // initialize timer

    // #ifdef HAVE_SETITIMER
    //   fgTimerInit( 1.0 / DEFAULT_TIMER_HZ, fgUpdateTimeDepCalcs );
    // #endif HAVE_SETITIMER
}

static const double alt_adjust_ft = 3.758099;
static const double alt_adjust_m = alt_adjust_ft * FEET_TO_METER;

// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void ) {
    FGState *f;
    fgTIME *t;
    static long remainder = 0;
    long elapsed, multi_loop;
    // int i;
    // double accum;
    static time_t last_time = 0;
    static int frames = 0;

    f = current_aircraft.fdm_state;
    t = &cur_time_params;

    FG_LOG( FG_ALL, FG_DEBUG, "Running Main Loop");
    FG_LOG( FG_ALL, FG_DEBUG, "======= ==== ====");

#if defined( ENABLE_LINUX_JOYSTICK )
    // Read joystick and update control settings
    fgJoystickRead();
#elif defined( ENABLE_GLUT_JOYSTICK )
    // Glut joystick support works by feeding a joystick handler
    // function to glut.  This is taken care of once in the joystick
    // init routine and we don't have to worry about it again.
#endif

    current_weather.Update();

    // Fix elevation.  I'm just sticking this here for now, it should
    // probably move eventually

    /* printf("Before - ground = %.2f  runway = %.2f  alt = %.2f\n",
	   scenery.cur_elev,
	   f->get_Runway_altitude() * FEET_TO_METER,
	   f->get_Altitude() * FEET_TO_METER); */

    if ( scenery.cur_elev > -9990 ) {
	if ( f->get_Altitude() * FEET_TO_METER < 
	     (scenery.cur_elev + alt_adjust_m - 3.0) ) {
	    // now set aircraft altitude above ground
	    printf("Current Altitude = %.2f < %.2f forcing to %.2f\n", 
		   f->get_Altitude() * FEET_TO_METER,
		   scenery.cur_elev + alt_adjust_m - 3.0,
		   scenery.cur_elev + alt_adjust_m );
	    fgFlightModelSetAltitude( current_options.get_flight_model(), 
				      scenery.cur_elev + alt_adjust_m );

	    FG_LOG( FG_ALL, FG_DEBUG, 
		    "<*> resetting altitude to " 
		    << f->get_Altitude() * FEET_TO_METER << " meters" );
	}
	f->set_Runway_altitude( scenery.cur_elev * METER_TO_FEET );
    }

    /* printf("Adjustment - ground = %.2f  runway = %.2f  alt = %.2f\n",
	   scenery.cur_elev,
	   f->get_Runway_altitude() * FEET_TO_METER,
	   f->get_Altitude() * FEET_TO_METER); */

    // update "time"
    fgTimeUpdate(f, t);

    // Get elapsed time (in usec) for this past frame
    elapsed = fgGetTimeInterval();
    FG_LOG( FG_ALL, FG_DEBUG, 
	    "Elapsed time interval is = " << elapsed 
	    << ", previous remainder is = " << remainder );

    // Calculate frame rate average
    if ( (t->cur_time != last_time) && (last_time > 0) ) {
	general.set_frame_rate( frames );
	FG_LOG( FG_ALL, FG_DEBUG, 
		"--> Frame rate is = " << general.get_frame_rate() );
	frames = 0;
    }
    last_time = t->cur_time;
    ++frames;

    /* old fps calculation
    if ( elapsed > 0 ) {
	accum = 0.0;
	for ( i = FG_FRAME_RATE_HISTORY - 2; i >= 0; i-- ) {
	    accum += g->frames[i];
	    // printf("frame[%d] = %.2f\n", i, g->frames[i]);
	    g->frames[i+1] = g->frames[i];
	}
	g->frames[0] = 1000.0 / (float)elapsed;
	// printf("frame[0] = %.2f\n", g->frames[0]);
	accum += g->frames[0];
	g->frame_rate = accum / (float)FG_FRAME_RATE_HISTORY;
	// printf("ave = %.2f\n", g->frame_rate);
    }
    */

    // Run flight model
    if ( ! use_signals ) {
	// Calculate model iterations needed for next frame
	elapsed += remainder;

	multi_loop = (int)(((double)elapsed * 0.000001) * DEFAULT_MODEL_HZ);
	remainder = elapsed - ((multi_loop*1000000) / DEFAULT_MODEL_HZ);
	FG_LOG( FG_ALL, FG_DEBUG, 
		"Model iterations needed = " << multi_loop
		<< ", new remainder = " << remainder );
	
	// flight model
	if ( multi_loop > 0 ) {
	    fgUpdateTimeDepCalcs(multi_loop, remainder);
	} else {
	    FG_LOG( FG_ALL, FG_INFO, "Elapsed time is zero ... we're zinging" );
	}
    }

    // Do any serial port work that might need to be done
    fgSerialProcess();

    // see if we need to load any new scenery tiles
    fgTileMgrUpdate();

    // Process/manage pending events
    global_events.Process();

    // Run audio scheduler
#ifdef ENABLE_AUDIO_SUPPORT
    if ( current_options.get_sound() && audio_sched->working() ) {

#   ifdef MICHAEL_JOHNSON_EXPERIMENTAL_ENGINE_AUDIO

	// note: all these factors are relative to the sample.  our
	// sample format should really contain a conversion factor so
	// that we can get prop speed right for arbitrary samples.
	// Note that for normal-size props, there is a point at which
	// the prop tips approach the speed of sound; that is a pretty
	// strong limit to how fast the prop can go.

	// multiplication factor is prime pitch control; add some log
	// component for verisimilitude

	double pitch = log((controls.get_throttle(0) * 14.0) + 1.0);
	//fprintf(stderr, "pitch1: %f ", pitch);
	if (controls.get_throttle(0) > 0.0 || f->v_rel_wind > 40.0) {
	    //fprintf(stderr, "rel_wind: %f ", f->v_rel_wind);
	    // only add relative wind and AoA if prop is moving
	    // or we're really flying at idle throttle
	    if (pitch < 5.4) {  // this needs tuning
		// prop tips not breaking sound barrier
		pitch += log(f->v_rel_wind + 0.8)/2;
	    } else {
		// prop tips breaking sound barrier
		pitch += log(f->v_rel_wind + 0.8)/10;
	    }
	    //fprintf(stderr, "pitch2: %f ", pitch);
	    //fprintf(stderr, "AoA: %f ", FG_Gamma_vert_rad);

	    // Angle of Attack next... -x^3(e^x) is my best guess Just
	    // need to calculate some reasonable scaling factor and
	    // then clamp it on the positive aoa (neg adj) side
	    double aoa = f->get_Gamma_vert_rad() * 2.2;
	    double tmp = 3.0;
	    double aoa_adj = pow(-aoa, tmp) * pow(M_E, aoa);
	    if (aoa_adj < -0.8) aoa_adj = -0.8;
	    pitch += aoa_adj;
	    //fprintf(stderr, "pitch3: %f ", pitch);

	    // don't run at absurdly slow rates -- not realistic
	    // and sounds bad to boot.  :-)
	    if (pitch < 0.8) pitch = 0.8;
	}
	//fprintf(stderr, "pitch4: %f\n", pitch);

	double volume = controls.get_throttle(0) * 1.15 + 0.3 +
	    log(f->v_rel_wind + 1.0)/14.0;
	// fprintf(stderr, "volume: %f\n", volume);

	pitch_envelope.setStep  ( 0, 0.01, pitch );
	volume_envelope.setStep ( 0, 0.01, volume );

#   else

       double param = controls.get_throttle( 0 ) * 2.0 + 1.0;
       pitch_envelope.setStep  ( 0, 0.01, param );
       volume_envelope.setStep ( 0, 0.01, param );

#   endif // experimental throttle patch

	audio_sched -> update();
    }
#endif

    // redraw display
    fgRenderFrame();

    FG_LOG( FG_ALL, FG_DEBUG, "" );
}


// This is the top level master main function that is registered as
// our idle funciton
//

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static void fgIdleFunction ( void ) {
    // printf("idle state == %d\n", idle_state);

    if ( idle_state == 0 ) {
	// Initialize the splash screen right away
	if ( current_options.get_splash_screen() ) {
	    fgSplashInit();
	}
	
	idle_state++;
    } else if ( idle_state == 1 ) {
	// Start the intro music
#if !defined(WIN32)
	if ( current_options.get_intro_music() ) {
	    string lockfile = "/tmp/mpg123.running";
	    string mp3file = current_options.get_fg_root() +
		"/Sounds/intro.mp3";
	    string command = "(touch " + lockfile + "; mpg123 " + mp3file +
		 "> /dev/null 2>&1; /bin/rm " + lockfile + ") &";
	    FG_LOG( FG_GENERAL, FG_INFO, 
		    "Starting intro music: " << mp3file );
	    system ( command.c_str() );
	}
#endif

	idle_state++;
    } else if ( idle_state == 2 ) {
	// These are a few miscellaneous things that aren't really
	// "subsystems" but still need to be initialized.

#ifdef USE_GLIDE
	if ( strstr ( general.get_glRenderer(), "Glide" ) ) {
	    grTexLodBiasValue ( GR_TMU0, 1.0 ) ;
	}
#endif

	idle_state++;
    } else if ( idle_state == 3 ) {
	// This is the top level init routine which calls all the
	// other subsystem initialization routines.  If you are adding
	// a subsystem to flight gear, its initialization call should
	// located in this routine.
	if( !fgInitSubsystems()) {
	    FG_LOG( FG_GENERAL, FG_ALERT,
		    "Subsystem initializations failed ..." );
	    exit(-1);
	}

	idle_state++;
    } else if ( idle_state == 4 ) {
	// setup OpenGL view parameters
	fgInitVisuals();

	if ( use_signals ) {
	    // init timer routines, signals, etc.  Arrange for an alarm
	    // signal to be generated, etc.
	    fgInitTimeDepCalcs();
	}

	idle_state++;
    } else if ( idle_state == 5 ) {

	idle_state++;
    } else if ( idle_state == 6 ) {
	// Initialize audio support
#ifdef ENABLE_AUDIO_SUPPORT

#if !defined(WIN32)
	if ( current_options.get_intro_music() ) {
	    // Let's wait for mpg123 to finish
	    string lockfile = "/tmp/mpg123.running";
	    struct stat stat_buf;

	    FG_LOG( FG_GENERAL, FG_INFO, 
		    "Waiting for mpg123 player to finish ..." );
	    while ( stat(lockfile.c_str(), &stat_buf) == 0 ) {
		// file exist, wait ...
		sleep(1);
		FG_LOG( FG_GENERAL, FG_INFO, ".");
	    }
	    FG_LOG( FG_GENERAL, FG_INFO, "");
	}
#endif // WIN32

	audio_sched = new slScheduler ( 8000 );
	audio_mixer = new smMixer;
	audio_mixer -> setMasterVolume ( 50 ) ;  /* 80% of max volume. */
	audio_sched -> setSafetyMargin ( 1.0 ) ;
	string slfile = current_options.get_fg_root() + "/Sounds/wasp.wav";

	s1 = new slSample ( (char *)slfile.c_str() );
	FG_LOG( FG_GENERAL, FG_INFO,
		"Rate = " << s1 -> getRate()
		<< "  Bps = " << s1 -> getBps()
		<< "  Stereo = " << s1 -> getStereo() );
	audio_sched -> loopSample ( s1 );

	if ( audio_sched->working() ) {
	    pitch_envelope.setStep  ( 0, 0.01, 0.6 );
	    volume_envelope.setStep ( 0, 0.01, 0.6 );

	    audio_sched -> addSampleEnvelope( s1, 0, 0, &
					      pitch_envelope,
					      SL_PITCH_ENVELOPE );
	    audio_sched -> addSampleEnvelope( s1, 0, 1, 
					      &volume_envelope,
					      SL_VOLUME_ENVELOPE );
	}

	// strcpy(slfile, path);
	// strcat(slfile, "thunder.wav");
	// s2 -> loadFile ( slfile );
	// s2 -> adjustVolume(0.5);
	// audio_sched -> playSample ( s2 );
#endif

	// sleep(1);
	idle_state = 1000;
    } 

    if ( idle_state == 1000 ) {
	// We've finished all our initialization steps, from now on we
	// run the main loop.

	fgMainLoop();
    } else {
	if ( current_options.get_splash_screen() == 1 ) {
	    fgSplashUpdate(0.0);
	}
    }
}


// Handle new window size or exposure
static void fgReshape( int width, int height ) {
    // Do this so we can call fgReshape(0,0) ourselves without having
    // to know what the values of width & height are.
    if ( (height > 0) && (width > 0) ) {
	if ( ! current_options.get_panel_status() ) {
	    current_view.set_win_ratio( (GLfloat) width / (GLfloat) height );
	} else {
	    current_view.set_win_ratio( (GLfloat) width / 
					((GLfloat) (height)*0.4232) );
	}
    }

    current_view.set_winWidth( width );
    current_view.set_winHeight( height );
    current_view.force_update_fov_math();

    // Inform gl of our view window size (now handled elsewhere)
    // xglViewport(0, 0, (GLint)width, (GLint)height);
    if ( idle_state == 1000 ) {
	// yes we've finished all our initializations and are running
	// the main loop, so this will now work without seg faulting
	// the system.
	current_view.UpdateViewParams();
	if ( current_options.get_panel_status() ) {
	    fgPanelReInit(0, 0, 1024, 768);
	}
    }
}


// Initialize GLUT and define a main window
int fgGlutInit( int *argc, char **argv ) {
    // GLUT will extract all glut specific options so later on we only
    // need wory about our own.
    xglutInit(argc, argv);

    // Define Display Parameters
    xglutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

    FG_LOG( FG_GENERAL, FG_INFO, "Opening a window: " <<
	    current_options.get_xsize() << "x" << current_options.get_ysize() );

    // Define initial window size
    xglutInitWindowSize( current_options.get_xsize(),
			 current_options.get_ysize() );

    // Initialize windows
    if ( current_options.get_game_mode() == 0 ) {
	// Open the regular window
	xglutCreateWindow("Flight Gear");
    } else {
	// Open the cool new 'game mode' window
	string game_mode_params = "width=" + current_options.get_xsize();
	game_mode_params += "height=" + current_options.get_ysize();
	game_mode_params += " bpp=16";
	cout << "game mode params = " << game_mode_params;
	glutGameModeString( game_mode_params.c_str() );
	glutEnterGameMode();
    }

    // This seems to be the absolute earliest in the init sequence
    // that these calls will return valid info.  Too bad it's after
    // we've already created and sized out window. :-(
    general.set_glVendor( (char *)glGetString ( GL_VENDOR ) );
    general.set_glRenderer( (char *)glGetString ( GL_RENDERER ) );
    general.set_glVersion( (char *)glGetString ( GL_VERSION ) );

    FG_LOG ( FG_GENERAL, FG_INFO, general.get_glRenderer() );

#if 0
    // try to determine if we should adjust the initial default
    // display resolution.  The options class defaults (is
    // initialized) to 640x480.
    string renderer = general.glRenderer;

    // currently we only know how to deal with Mesa/Glide/Voodoo cards
    if ( renderer.find( "Glide" ) != string::npos ) {
	FG_LOG( FG_GENERAL, FG_INFO, "Detected a Glide driver" );
	if ( renderer.find( "FB/8" ) != string::npos ) {
	    // probably a voodoo-2
	    if ( renderer.find( "TMU/SLI" ) != string::npos ) {
		// probably two SLI'd Voodoo-2's
		current_options.set_xsize( 1024 );
		current_options.set_ysize( 768 );
		FG_LOG( FG_GENERAL, FG_INFO,
			"It looks like you have two sli'd voodoo-2's." << endl
			<< "upgrading your win resolution to 1024 x 768" );
		glutReshapeWindow(1024, 768);
	    } else {
		// probably a single non-SLI'd Voodoo-2
		current_options.set_xsize( 800 );
		current_options.set_ysize( 600 );
		FG_LOG( FG_GENERAL, FG_INFO,
			"It looks like you have a voodoo-2." << endl
			<< "upgrading your win resolution to 800 x 600" );
		glutReshapeWindow(800, 600);
	    }
	} else if ( renderer.find( "FB/2" ) != string::npos ) {
	    // probably a voodoo-1, stick with the default
	}
    } else {
	// we have no special knowledge of this card, stick with the default
    }
#endif

    return(1);
}


// Initialize GLUT event handlers
int fgGlutInitEvents( void ) {
    // call fgReshape() on window resizes
    xglutReshapeFunc( fgReshape );

    // call GLUTkey() on keyboard event
    xglutKeyboardFunc( GLUTkey );
    glutSpecialFunc( GLUTspecialkey );

    // call guiMouseFunc() whenever our little rodent is used
    glutMouseFunc ( guiMouseFunc );
    glutMotionFunc (guiMotionFunc );
    glutPassiveMotionFunc (guiMotionFunc );

    // call fgMainLoop() whenever there is
    // nothing else to do
    xglutIdleFunc( fgIdleFunction );

    // draw the scene
    xglutDisplayFunc( fgRenderFrame );

    return(1);
}


// Main ...
int main( int argc, char **argv ) {
    FGState *f;

    f = current_aircraft.fdm_state;

#ifdef HAVE_BC5PLUS
    _control87(MCW_EM, MCW_EM);  /* defined in float.h */
#endif

    // Initialize the [old] debugging output system
    // fgInitDebug();

    // set default log levels
    fglog().setLogLevels( FG_ALL, FG_INFO );

    FG_LOG( FG_GENERAL, FG_INFO, "Flight Gear:  Version " << VERSION << endl );

    string root;

    FG_LOG( FG_GENERAL, FG_INFO, "General Initialization" );
    FG_LOG( FG_GENERAL, FG_INFO, "======= ==============" );

    // Attempt to locate and parse a config file
    // First check fg_root
    string config = current_options.get_fg_root() + "/system.fgfsrc";
    current_options.parse_config_file( config );

    // Next check home directory
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
	config = envp;
	config += "/.fgfsrc";
	current_options.parse_config_file( config );
    }

    // Parse remaining command line options
    // These will override anything specified in a config file
    if ( current_options.parse_command_line(argc, argv) !=
	                              fgOPTIONS::FG_OPTIONS_OK )
    {
	// Something must have gone horribly wrong with the command
	// line parsing or maybe the user just requested help ... :-)
	current_options.usage();
	// FG_LOG( FG_GENERAL, FG_ALERT, "\nExiting ...");
	cout << endl << "Exiting ..." << endl;
	exit(-1);
    }
    
    // Initialize the Window/Graphics environment.
    if( !fgGlutInit(&argc, argv) ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "GLUT initialization failed ..." );
	exit(-1);
    }

    // Initialize the various GLUT Event Handlers.
    if( !fgGlutInitEvents() ) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"GLUT event handler initialization failed ..." );
	exit(-1);
    }

    // First do some quick general initializations
    if( !fgInitGeneral()) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"General initializations failed ..." );
	exit(-1);
    }

    // Init the user interface (we need to do this before passing off
    // control to glut
    guiInit();

    // pass control off to the master GLUT event handler
    glutMainLoop();

    // we never actually get here ... but just in case ... :-)
    return(0);
}


// $Log$
// Revision 1.81  1999/01/19 20:57:03  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.80  1999/01/09 13:37:40  curt
// Convert fgTIMESTAMP to FGTimeStamp which holds usec instead of ms.
//
// Revision 1.79  1999/01/08 03:23:56  curt
// Beginning work on compensating for sim time vs. real world time "jitter".
//
// Revision 1.78  1999/01/07 20:25:08  curt
// Updated struct fgGENERAL to class FGGeneral.
//
// Revision 1.77  1998/12/18 23:40:55  curt
// New frame rate counting mechanism.
//
// Revision 1.76  1998/12/11 20:26:26  curt
// Fixed view frustum culling accuracy bug so we can look out the sides and
// back without tri-stripes dropping out.
//
// Revision 1.75  1998/12/09 18:50:23  curt
// Converted "class fgVIEW" to "class FGView" and updated to make data
// members private and make required accessor functions.
//
// Revision 1.74  1998/12/06 14:52:54  curt
// Fixed a problem with the initial starting altitude.  "v->abs_view_pos" wasn't
// being calculated correctly at the beginning causing the first terrain
// intersection to fail, returning a ground altitude of zero, causing the plane
// to free fall for one frame, until the ground altitude was corrected, but now
// being under the ground we got a big bounce and the plane always ended up
// upside down.
//
// Revision 1.73  1998/12/06 13:51:22  curt
// Turned "struct fgWEATHER" into "class FGWeather".
//
// Revision 1.72  1998/12/05 15:54:18  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.71  1998/12/05 14:19:51  curt
// Looking into a problem with cur_view_params.abs_view_pos initialization.
//
// Revision 1.70  1998/12/03 01:17:14  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.69  1998/11/23 20:51:26  curt
// Fiddling with when I can get info from the opengl driver.
//
// Revision 1.68  1998/11/20 01:02:35  curt
// Try to detect Mesa/Glide/Voodoo and chose the appropriate resolution.
//
// Revision 1.67  1998/11/16 13:59:58  curt
// Added pow() macro bug work around.
// Added support for starting FGFS at various resolutions.
// Added some initial serial port support.
// Specify default log levels in main().
//
// Revision 1.66  1998/11/11 00:24:00  curt
// Added Michael Johnson's audio patches for testing.
// Also did a few tweaks to avoid numerical problems when starting at a place
// with no (or bogus) scenery.
//
// Revision 1.65  1998/11/09 23:39:22  curt
// Tweaks for the instrument panel.
//
// Revision 1.64  1998/11/07 19:07:09  curt
// Enable release builds using the --without-logging option to the configure
// script.  Also a couple log message cleanups, plus some C to C++ comment
// conversion.
//
// Revision 1.63  1998/11/06 21:18:08  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.62  1998/10/27 02:14:35  curt
// Changes to support GLUT joystick routines as fall back.
//
// Revision 1.61  1998/10/25 14:08:47  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.60  1998/10/25 10:57:18  curt
// Changes to use the new joystick library if it is available.
//
// Revision 1.59  1998/10/17 01:34:21  curt
// C++ ifying ...
//
// Revision 1.58  1998/10/16 23:27:52  curt
// C++-ifying.
//
// Revision 1.57  1998/10/16 00:54:00  curt
// Converted to Point3D class.
//
// Revision 1.56  1998/10/02 12:46:47  curt
// Added an "auto throttle"
//
// Revision 1.55  1998/09/29 14:58:18  curt
// Use working() instead of !not_working() for audio.
//
// Revision 1.54  1998/09/29 02:03:38  curt
// Autopilot mods.
//
// Revision 1.53  1998/09/26 13:18:35  curt
// Check if audio "working()" before doing audio manipulations.
//
// Revision 1.52  1998/09/25 16:02:07  curt
// Added support for pitch and volume envelopes and tied them to the
// throttle setting.
//
// Revision 1.51  1998/09/15 04:27:28  curt
// Changes for new Astro code.
//
// Revision 1.50  1998/09/15 02:09:24  curt
// Include/fg_callback.hxx
//   Moved code inline to stop g++ 2.7 from complaining.
//
// Simulator/Time/event.[ch]xx
//   Changed return type of fgEVENT::printStat().  void caused g++ 2.7 to
//   complain bitterly.
//
// Minor bugfix and changes.
//
// Simulator/Main/GLUTmain.cxx
//   Added missing type to idle_state definition - eliminates a warning.
//
// Simulator/Main/fg_init.cxx
//   Changes to airport lookup.
//
// Simulator/Main/options.cxx
//   Uses fg_gzifstream when loading config file.
//
// Revision 1.49  1998/09/09 16:25:39  curt
// Only use GLUT_STENCIL if the instument panel has been requested.
//
// Revision 1.48  1998/08/28 18:15:03  curt
// Added new cockpit code from Friedemann Reinhard
// <mpt218@faupt212.physik.uni-erlangen.de>
//
// Revision 1.47  1998/08/27 17:02:04  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.46  1998/08/22  14:49:56  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.45  1998/08/20 20:32:31  curt
// Reshuffled some of the code in and around views.[ch]xx
//
// Revision 1.44  1998/08/20 15:10:33  curt
// Added GameGLUT support.
//
// Revision 1.43  1998/08/12 21:01:47  curt
// Master volume from 30% -> 80%
//
// Revision 1.42  1998/07/30 23:48:25  curt
// Output position & orientation when pausing.
// Eliminated libtool use.
// Added options to specify initial position and orientation.
// Changed default fov to 55 degrees.
// Added command line option to start in paused or unpaused state.
//
// Revision 1.41  1998/07/27 18:41:24  curt
// Added a pause command "p"
// Fixed some initialization order problems between pui and glut.
// Added an --enable/disable-sound option.
//
// Revision 1.40  1998/07/24 21:56:59  curt
// Set near clip plane to 0.5 meters when close to the ground.  Also, let the view get a bit closer to the ground before hitting the hard limit.
//
// Revision 1.39  1998/07/24 21:39:08  curt
// Debugging output tweaks.
// Cast glGetString to (char *) to avoid compiler errors.
// Optimizations to fgGluLookAt() by Norman Vine.
//
// Revision 1.38  1998/07/22 21:40:43  curt
// Clear to adjusted fog color (for sunrise/sunset effects)
// Make call to fog sunrise/sunset adjustment method.
// Add a stdc++ library bug work around to fg_init.cxx
//
// Revision 1.37  1998/07/20 12:49:44  curt
// Tweaked color buffer clearing defaults.  We clear the color buffer if we
// are doing textures.  Assumptions:  If we are doing textures we have hardware
// support that can clear the color buffer for "free."  If we are doing software
// rendering with textures, then the extra clear time gets lost in the noise.
//
// Revision 1.36  1998/07/16 17:33:35  curt
// "H" / "h" now control hud brightness as well with off being one of the
//   states.
// Better checking for xmesa/fx 3dfx fullscreen/window support for deciding
//   whether or not to build in the feature.
// Translucent menu support.
// HAVE_AUDIO_SUPPORT -> ENABLE_AUDIO_SUPPORT
// Use fork() / wait() for playing mp3 init music in background under unix.
// Changed default tile diameter to 5.
//
// Revision 1.35  1998/07/13 21:01:36  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.34  1998/07/13 15:32:37  curt
// Clear color buffer if drawing wireframe.
// When specifying and airport, start elevation at -1000 and let the system
// position you at ground level.
//
// Revision 1.33  1998/07/12 03:14:42  curt
// Added ground collision detection.
// Did some serious horsing around to be able to "hug" the ground properly
//   and still be able to take off.
// Set the near clip plane to 1.0 meters when less than 10 meters above the
//   ground.
// Did some serious horsing around getting the initial airplane position to be
//   correct based on rendered terrain elevation.
// Added a little cheat/hack that will prevent the view position from ever
//   dropping below the terrain, even when the flight model doesn't quite
//   put you as high as you'd like.
//
// Revision 1.32  1998/07/08 14:45:07  curt
// polar3d.h renamed to polar3d.hxx
// vector.h renamed to vector.hxx
// updated audio support so it waits to create audio classes (and tie up
//   /dev/dsp) until the mpg123 player is finished.
//
// Revision 1.31  1998/07/06 21:34:17  curt
// Added an enable/disable splash screen option.
// Added an enable/disable intro music option.
// Added an enable/disable instrument panel option.
// Added an enable/disable mouse pointer option.
// Added using namespace std for compilers that support this.
//
// Revision 1.30  1998/07/06 02:42:03  curt
// Added support for switching between fullscreen and window mode for
// Mesa/3dfx/glide.
//
// Added a basic splash screen.  Restructured the main loop and top level
// initialization routines to do this.
//
// Hacked in some support for playing a startup mp3 sound file while rest
// of sim initializes.  Currently only works in Unix using the mpg123 player.
// Waits for the mpg123 player to finish before initializing internal
// sound drivers.
//
// Revision 1.29  1998/07/04 00:52:22  curt
// Add my own version of gluLookAt() (which is nearly identical to the
// Mesa/glu version.)  But, by calculating the Model View matrix our selves
// we can save this matrix without having to read it back in from the video
// card.  This hopefully allows us to save a few cpu cycles when rendering
// out the fragments because we can just use glLoadMatrixd() with the
// precalculated matrix for each tile rather than doing a push(), translate(),
// pop() for every fragment.
//
// Panel status defaults to off for now until it gets a bit more developed.
//
// Extract OpenGL driver info on initialization.
//
// Revision 1.28  1998/06/27 16:54:32  curt
// Replaced "extern displayInstruments" with a entry in fgOPTIONS.
// Don't change the view port when displaying the panel.
//
// Revision 1.27  1998/06/17 21:35:10  curt
// Refined conditional audio support compilation.
// Moved texture parameter setup calls to ../Scenery/materials.cxx
// #include <string.h> before various STL includes.
// Make HUD default state be enabled.
//
// Revision 1.26  1998/06/13 00:40:32  curt
// Tweaked fog command line options.
//
// Revision 1.25  1998/06/12 14:27:26  curt
// Pui -> PUI, Gui -> GUI.
//
// Revision 1.24  1998/06/12 00:57:39  curt
// Added support for Pui/Gui.
// Converted fog to GL_FOG_EXP2.
// Link to static simulator parts.
// Update runfg.bat to try to be a little smarter.
//
// Revision 1.23  1998/06/08 17:57:04  curt
// Minor sound/startup position tweaks.
//
// Revision 1.22  1998/06/05 18:18:40  curt
// A bit of fiddling with audio ...
//
// Revision 1.21  1998/06/03 22:01:06  curt
// Tweaking sound library usage.
//
// Revision 1.20  1998/06/03 00:47:11  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.19  1998/06/01 17:54:40  curt
// Added Linux audio support.
// avoid glClear( COLOR_BUFFER_BIT ) when not using it to set the sky color.
// map stl tweaks.
//
// Revision 1.18  1998/05/29 20:37:19  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.17  1998/05/22 21:28:52  curt
// Modifications to use the new fgEVENT_MGR class.
//
// Revision 1.16  1998/05/20 20:51:33  curt
// Tweaked smooth shaded texture lighting properties.
// Converted fgLIGHT to a C++ class.
//
// Revision 1.15  1998/05/16 13:08:34  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.14  1998/05/13 18:29:57  curt
// Added a keyboard binding to dynamically adjust field of view.
// Added a command line option to specify fov.
// Adjusted terrain color.
// Root path info moved to fgOPTIONS.
// Added ability to parse options out of a config file.
//
// Revision 1.13  1998/05/11 18:18:15  curt
// For flat shading use "glHint (GL_FOG_HINT, GL_FASTEST )"
//
// Revision 1.12  1998/05/07 23:14:15  curt
// Added "D" key binding to set autopilot heading.
// Made frame rate calculation average out over last 10 frames.
// Borland C++ floating point exception workaround.
// Added a --tile-radius=n option.
//
// Revision 1.11  1998/05/06 03:16:23  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.10  1998/05/03 00:47:31  curt
// Added an option to enable/disable full-screen mode.
//
// Revision 1.9  1998/04/30 12:34:17  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.8  1998/04/28 01:20:21  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.7  1998/04/26 05:10:02  curt
// "struct fgLIGHT" -> "fgLIGHT" because fgLIGHT is typedef'd.
//
// Revision 1.6  1998/04/25 22:06:30  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.5  1998/04/25 20:24:01  curt
// Cleaned up initialization sequence to eliminate interdependencies
// between sun position, lighting, and view position.  This creates a
// valid single pass initialization path.
//
// Revision 1.4  1998/04/24 14:19:30  curt
// Fog tweaks.
//
// Revision 1.3  1998/04/24 00:49:18  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
// Revision 1.2  1998/04/22 13:25:41  curt
// C++ - ifing the code.
// Starting a bit of reorganization of lighting code.
//
// Revision 1.1  1998/04/21 17:02:39  curt
// Prepairing for C++ integration.
//
// Revision 1.71  1998/04/18 04:11:26  curt
// Moved fg_debug to it's own library, added zlib support.
//
// Revision 1.70  1998/04/14 02:21:02  curt
// Incorporated autopilot heading hold contributed by:  Jeff Goeke-Smith
// <jgoeke@voyager.net>
//
// Revision 1.69  1998/04/08 23:35:34  curt
// Tweaks to Gnu automake/autoconf system.
//
// Revision 1.68  1998/04/03 22:09:03  curt
// Converting to Gnu autoconf system.
//
// Revision 1.67  1998/03/23 21:24:37  curt
// Source code formating tweaks.
//
// Revision 1.66  1998/03/14 00:31:20  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.65  1998/03/09 22:45:57  curt
// Minor tweaks for building on sparc platform.
//
// Revision 1.64  1998/02/20 00:16:23  curt
// Thursday's tweaks.
//
// Revision 1.63  1998/02/16 16:17:39  curt
// Minor tweaks.
//
// Revision 1.62  1998/02/16 13:39:42  curt
// Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
// tiles to occasionally be missing.
//
// Revision 1.61  1998/02/12 21:59:46  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.60  1998/02/11 02:50:40  curt
// Minor changes.
//
// Revision 1.59  1998/02/09 22:56:54  curt
// Removed "depend" files from cvs control.  Other minor make tweaks.
//
// Revision 1.58  1998/02/09 15:07:49  curt
// Minor tweaks.
//
// Revision 1.57  1998/02/07 15:29:40  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.56  1998/02/03 23:20:23  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.55  1998/02/02 20:53:58  curt
// Incorporated Durk's changes.
//
// Revision 1.54  1998/01/31 00:43:10  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.53  1998/01/27 18:35:54  curt
// Minor tweaks.
//
// Revision 1.52  1998/01/27 00:47:56  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.51  1998/01/26 15:57:05  curt
// Tweaks for dynamic scenery development.
//
// Revision 1.50  1998/01/19 19:27:07  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.49  1998/01/19 18:40:31  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.48  1998/01/19 18:35:46  curt
// Minor tweaks and fixes for cygwin32.
//
// Revision 1.47  1998/01/13 00:23:08  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.46  1998/01/08 02:22:06  curt
// Beginning to integrate Tile management subsystem.
//
// Revision 1.45  1998/01/07 03:18:55  curt
// Moved astronomical stuff from .../Src/Scenery to .../Src/Astro/
//
// Revision 1.44  1997/12/30 22:22:31  curt
// Further integration of event manager.
//
// Revision 1.43  1997/12/30 20:47:43  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.42  1997/12/30 16:36:47  curt
// Merged in Durk's changes ...
//
// Revision 1.41  1997/12/30 13:06:56  curt
// A couple lighting tweaks ...
//
// Revision 1.40  1997/12/30 01:38:37  curt
// Switched back to per vertex normals and smooth shading for terrain.
//
// Revision 1.39  1997/12/22 23:45:45  curt
// First stab at sunset/sunrise sky glow effects.
//
// Revision 1.38  1997/12/22 04:14:28  curt
// Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
//
// Revision 1.37  1997/12/19 23:34:03  curt
// Lot's of tweaking with sky rendering and lighting.
//
// Revision 1.36  1997/12/19 16:44:57  curt
// Working on scene rendering order and options.
//
// Revision 1.35  1997/12/18 23:32:32  curt
// First stab at sky dome actually starting to look reasonable. :-)
//
// Revision 1.34  1997/12/17 23:13:34  curt
// Began working on rendering a sky.
//
// Revision 1.33  1997/12/15 23:54:45  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.32  1997/12/15 20:59:08  curt
// Misc. tweaks.
//
// Revision 1.31  1997/12/12 21:41:25  curt
// More light/material property tweaking ... still a ways off.
//
// Revision 1.30  1997/12/12 19:52:47  curt
// Working on lightling and material properties.
//
// Revision 1.29  1997/12/11 04:43:54  curt
// Fixed sun vector and lighting problems.  I thing the moon is now lit
// correctly.
//
// Revision 1.28  1997/12/10 22:37:45  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.27  1997/12/09 05:11:54  curt
// Working on tweaking lighting.
//
// Revision 1.26  1997/12/09 04:25:29  curt
// Working on adding a global lighting params structure.
//
// Revision 1.25  1997/12/08 22:54:09  curt
// Enabled GL_CULL_FACE.
//
// Revision 1.24  1997/11/25 19:25:32  curt
// Changes to integrate Durk's moon/sun code updates + clean up.
//
// Revision 1.23  1997/11/15 18:16:34  curt
// minor tweaks.
//
// Revision 1.22  1997/10/30 12:38:41  curt
// Working on new scenery subsystem.
//
// Revision 1.21  1997/09/23 00:29:38  curt
// Tweaks to get things to compile with gcc-win32.
//
// Revision 1.20  1997/09/22 14:44:19  curt
// Continuing to try to align stars correctly.
//
// Revision 1.19  1997/09/18 16:20:08  curt
// At dusk/dawn add/remove stars in stages.
//
// Revision 1.18  1997/09/16 22:14:51  curt
// Tweaked time of day lighting equations.  Don't draw stars during the day.
//
// Revision 1.17  1997/09/16 15:50:29  curt
// Working on star alignment and time issues.
//
// Revision 1.16  1997/09/13 02:00:06  curt
// Mostly working on stars and generating sidereal time for accurate star
// placement.
//
// Revision 1.15  1997/09/05 14:17:27  curt
// More tweaking with stars.
//
// Revision 1.14  1997/09/05 01:35:53  curt
// Working on getting stars right.
//
// Revision 1.13  1997/09/04 02:17:34  curt
// Shufflin' stuff.
//
// Revision 1.12  1997/08/27 21:32:24  curt
// Restructured view calculation code.  Added stars.
//
// Revision 1.11  1997/08/27 03:30:16  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.10  1997/08/25 20:27:22  curt
// Merged in initial HUD and Joystick code.
//
// Revision 1.9  1997/08/22 21:34:39  curt
// Doing a bit of reorganizing and house cleaning.
//
// Revision 1.8  1997/08/19 23:55:03  curt
// Worked on better simulating real lighting.
//
// Revision 1.7  1997/08/16 12:22:38  curt
// Working on improving the lighting/shading.
//
// Revision 1.6  1997/08/13 20:24:56  curt
// Changes due to changing sunpos interface.
//
// Revision 1.5  1997/08/06 21:08:32  curt
// Sun position now really* works (I think) ... I still have sun time warping
// code in place, probably should remove it soon.
//
// Revision 1.4  1997/08/06 15:41:26  curt
// Working on correct sun position.
//
// Revision 1.3  1997/08/06 00:24:22  curt
// Working on correct real time sun lighting.
//
// Revision 1.2  1997/08/04 20:25:15  curt
// Organizational tweaking.
//
// Revision 1.1  1997/08/02 18:45:00  curt
// Renamed GLmain.c GLUTmain.c
//
// Revision 1.43  1997/08/02 16:23:47  curt
// Misc. tweaks.
//
// Revision 1.42  1997/08/01 19:43:33  curt
// Making progress with coordinate system overhaul.
//
// Revision 1.41  1997/07/31 22:52:37  curt
// Working on redoing internal coordinate systems & scenery transformations.
//
// Revision 1.40  1997/07/30 16:12:42  curt
// Moved fg_random routines from Util/ to Math/
//
// Revision 1.39  1997/07/21 14:45:01  curt
// Minor tweaks.
//
// Revision 1.38  1997/07/19 23:04:47  curt
// Added an initial weather section.
//
// Revision 1.37  1997/07/19 22:34:02  curt
// Moved PI definitions to ../constants.h
// Moved random() stuff to ../Utils/ and renamed fg_random()
//
// Revision 1.36  1997/07/18 23:41:25  curt
// Tweaks for building with Cygnus Win32 compiler.
//
// Revision 1.35  1997/07/18 14:28:34  curt
// Hacked in some support for wind/turbulence.
//
// Revision 1.34  1997/07/16 20:04:48  curt
// Minor tweaks to aid Win32 port.
//
// Revision 1.33  1997/07/12 03:50:20  curt
// Added an #include <Windows32/Base.h> to help compiling for Win32
//
// Revision 1.32  1997/07/11 03:23:18  curt
// Solved some scenery display/orientation problems.  Still have a positioning
// (or transformation?) problem.
//
// Revision 1.31  1997/07/11 01:29:58  curt
// More tweaking of terrian floor.
//
// Revision 1.30  1997/07/10 04:26:37  curt
// We now can interpolated ground elevation for any position in the grid.  We
// can use this to enforce a "hard" ground.  We still need to enforce some
// bounds checking so that we don't try to lookup data points outside the
// grid data set.
//
// Revision 1.29  1997/07/09 21:31:12  curt
// Working on making the ground "hard."
//
// Revision 1.28  1997/07/08 18:20:12  curt
// Working on establishing a hard ground.
//
// Revision 1.27  1997/07/07 20:59:49  curt
// Working on scenery transformations to enable us to fly fluidly over the
// poles with no discontinuity/distortion in scenery.
//
// Revision 1.26  1997/07/05 20:43:34  curt
// renamed mat3 directory to Math so we could add other math related routines.
//
// Revision 1.25  1997/06/29 21:19:17  curt
// Working on scenery management system.
//
// Revision 1.24  1997/06/26 22:14:53  curt
// Beginning work on a scenery management system.
//
// Revision 1.23  1997/06/26 19:08:33  curt
// Restructuring make, adding automatic "make dep" support.
//
// Revision 1.22  1997/06/25 15:39:47  curt
// Minor changes to compile with rsxnt/win32.
//
// Revision 1.21  1997/06/22 21:44:41  curt
// Working on intergrating the VRML (subset) parser.
//
// Revision 1.20  1997/06/21 17:12:53  curt
// Capitalized subdirectory names.
//
// Revision 1.19  1997/06/18 04:10:31  curt
// A couple more runway tweaks ...
//
// Revision 1.18  1997/06/18 02:21:24  curt
// Hacked in a runway
//
// Revision 1.17  1997/06/17 16:51:58  curt
// Timer interval stuff now uses gettimeofday() instead of ftime()
//
// Revision 1.16  1997/06/17 04:19:16  curt
// More timer related tweaks with respect to view direction changes.
//
// Revision 1.15  1997/06/17 03:41:10  curt
// Nonsignal based interval timing is now working.
// This would be a good time to look at cleaning up the code structure a bit.
//
// Revision 1.14  1997/06/16 19:32:51  curt
// Starting to add general timer support.
//
// Revision 1.13  1997/06/02 03:40:06  curt
// A tiny bit more view tweaking.
//
// Revision 1.12  1997/06/02 03:01:38  curt
// Working on views (side, front, back, transitions, etc.)
//
// Revision 1.11  1997/05/31 19:16:25  curt
// Elevator trim added.
//
// Revision 1.10  1997/05/31 04:13:52  curt
// WE CAN NOW FLY!!!
//
// Continuing work on the LaRCsim flight model integration.
// Added some MSFS-like keyboard input handling.
//
// Revision 1.9  1997/05/30 19:27:01  curt
// The LaRCsim flight model is starting to look like it is working.
//
// Revision 1.8  1997/05/30 03:54:10  curt
// Made a bit more progress towards integrating the LaRCsim flight model.
//
// Revision 1.7  1997/05/29 22:39:49  curt
// Working on incorporating the LaRCsim flight model.
//
// Revision 1.6  1997/05/29 12:31:39  curt
// Minor tweaks, moving towards general flight model integration.
//
// Revision 1.5  1997/05/29 02:33:23  curt
// Updated to reflect changing interfaces in other "modules."
//
// Revision 1.4  1997/05/27 17:44:31  curt
// Renamed & rearranged variables and routines.   Added some initial simple
// timer/alarm routines so the flight model can be updated on a regular 
// interval.
//
// Revision 1.3  1997/05/23 15:40:25  curt
// Added GNU copyright headers.
// Fog now works!
//
// Revision 1.2  1997/05/23 00:35:12  curt
// Trying to get fog to work ...
//
// Revision 1.1  1997/05/21 15:57:51  curt
// Renamed due to added GLUT support.
//
// Revision 1.3  1997/05/19 18:22:42  curt
// Parameter tweaking ... starting to stub in fog support.
//
// Revision 1.2  1997/05/17 00:17:34  curt
// Trying to stub in support for standard OpenGL.
//
// Revision 1.1  1997/05/16 16:05:52  curt
// Initial revision.
//
