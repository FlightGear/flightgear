// main.cxx -- top level sim routines
//
// Written by Curtis Olson for OpenGL, started May 1997.
//
// Copyright (C) 1997 - 1999  Curtis L. Olson  - curt@flightgear.org
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


#define MICHAEL_JOHNSON_EXPERIMENTAL_ENGINE_AUDIO


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
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

#include <pu.h>			// plib include
#include <ssg.h>		// plib include

#ifdef ENABLE_AUDIO_SUPPORT
#  include <sl.h>		// plib include
#  include <sm.h>		// plib include
#endif

#include <Include/fg_constants.h>  // for VERSION
#include <Include/general.hxx>

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Astro/sky.hxx>
#include <Astro/stars.hxx>
#include <Astro/solarsystem.hxx>

#include <Autopilot/autopilot.hxx>
#include <Cockpit/cockpit.hxx>
#include <GUI/gui.h>
#include <Joystick/joystick.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Math/fg_random.h>
#include <Misc/fgpath.hxx>
#ifdef FG_NETWORK_OLK
#include <Network/network.h>
#endif
#include <Objects/materialmgr.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>
#include <Time/fg_timer.hxx>
#include <Time/sunpos.hxx>

#ifdef FG_OLD_WEATHER
#  include <Weather/weather.hxx>
#endif

#include "fg_init.hxx"
#include "keyboard.hxx"
#include "options.hxx"
#include "splash.hxx"
#include "views.hxx"
#include "fg_serial.hxx"


// -dw- use custom sioux settings so I can see output window
#ifdef MACOS
#  ifndef FG_NDEBUG
#    include <sioux.h> // settings for output window
#  endif
#  include <console.h>
#endif


// This is a record containing a bit of global housekeeping information
FGGeneral general;

// Specify our current idle function state.  This is used to run all
// our initializations out of the glutIdleLoop() so that we can get a
// splash screen up and running right away.
static int idle_state = 0;

// Another hack
int use_signals = 0;

// Global structures for the Audio library
#ifdef ENABLE_AUDIO_SUPPORT
slEnvelope pitch_envelope ( 1, SL_SAMPLE_ONE_SHOT ) ;
slEnvelope volume_envelope ( 1, SL_SAMPLE_ONE_SHOT ) ;
slScheduler *audio_sched;
smMixer *audio_mixer;
slSample *s1;
slSample *s2;
#endif


// ssg variables
ssgRoot *scene = NULL;
ssgBranch *terrain = NULL;
ssgSelector *penguin_sel = NULL;
ssgTransform *penguin_pos = NULL;

// current fdm/position used for view
FGInterface cur_view_fdm;

// hack
sgMat4 copy_of_ssgOpenGLAxisSwapMatrix =
{
  {  1.0f,  0.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f, -1.0f,  0.0f },
  {  0.0f,  1.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f,  0.0f,  1.0f }
} ;

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
void fgInitVisuals( void ) {
    fgLIGHT *l;

    l = &cur_light_params;

#ifndef GLUT_WRONG_VERSION
    // Go full screen if requested ...
    if ( current_options.get_fullscreen() ) {
	glutFullScreen();
    }
#endif

    // If enabled, normal vectors specified with glNormal are scaled
    // to unit length after transformation.  See glNormal.
    // xglEnable( GL_NORMALIZE );

    xglEnable( GL_LIGHTING );
    xglEnable( GL_LIGHT0 );
    xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

    sgVec3 sunpos;
    sgSetVec3( sunpos, l->sun_vec[0], l->sun_vec[1], l->sun_vec[2] );
    ssgGetLight( 0 ) -> setPosition( sunpos );

    // xglFogi (GL_FOG_MODE, GL_LINEAR);
    xglFogi (GL_FOG_MODE, GL_EXP2);
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


// Update all Visuals (redraws anything graphics related)
void fgRenderFrame( void ) {
    fgLIGHT *l = &cur_light_params;
    FGTime *t = FGTime::cur_time_params;
    // FGView *v = &current_view;

    double angle;
    // GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat terrain_color[4] = { 0.54, 0.44, 0.29, 1.0 };
    // GLfloat mat_shininess[] = { 10.0 };
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
	// cout << "before pilot_view update" << endl;
	pilot_view.UpdateViewParams(*cur_fdm_state);
	// cout << "after pilot_view update" << endl;
	current_view.UpdateViewParams(cur_view_fdm);

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

	// I really should create a derived ssg node or use a call
	// back or something so that I can draw the sky within the
	// ssgCullandDraw() function, but for now I just mimic what
	// ssg does to set up the model view matrix
	xglMatrixMode(GL_MODELVIEW);
	xglLoadIdentity();
	sgMat4 vm_tmp, view_mat;
	sgTransposeNegateMat4 ( vm_tmp, current_view.VIEW ) ;
	sgCopyMat4( view_mat, copy_of_ssgOpenGLAxisSwapMatrix ) ;
	sgPreMultMat4( view_mat, vm_tmp ) ;
	xglLoadMatrixf( (float *)view_mat );

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
	Point3D view_pos = current_view.get_view_pos();
	xglTranslatef( view_pos.x(), view_pos.y(), view_pos.z() );
	// Rotate based on gst (sidereal time)
	// note: constant should be 15.041085, Curt thought it was 15
	angle = t->getGst() * 15.041085;
	// printf("Rotating astro objects by %.2f degrees\n",angle);
	xglRotatef( angle, 0.0, 0.0, -1.0 );

	// draw stars and planets
	fgStarsRender();
	xglDisable( GL_COLOR_MATERIAL ); // just to make sure ...
	xglEnable( GL_CULL_FACE ); // for moon
	xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );
	//xglEnable(GL_DEPTH_TEST);
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
	    xglFogi( GL_FOG_MODE, GL_EXP2 );
	    xglFogfv( GL_FOG_COLOR, l->adj_fog_color );
	}
	// set lighting parameters
	xglLightfv( GL_LIGHT0, GL_AMBIENT, l->scene_ambient );
	xglLightfv( GL_LIGHT0, GL_DIFFUSE, l->scene_diffuse );
	// xglLightfv(GL_LIGHT0, GL_SPECULAR, white );
	
	if ( current_options.get_textures() ) {
	    // texture parameters
	    xglEnable( GL_TEXTURE_2D );
	    xglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
	    xglHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;
	    // set base color (I don't think this is doing anything here)
	    xglMaterialfv (GL_FRONT, GL_AMBIENT, white);
	    xglMaterialfv (GL_FRONT, GL_DIFFUSE, white);
	    // xglMaterialfv (GL_FRONT, GL_SPECULAR, white);
	    // xglMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);
	} else {
	    xglDisable( GL_TEXTURE_2D );
	    xglMaterialfv (GL_FRONT, GL_AMBIENT, terrain_color);
	    xglMaterialfv (GL_FRONT, GL_DIFFUSE, terrain_color);
	    // xglMaterialfv (GL_FRONT, GL_AMBIENT, white);
	    // xglMaterialfv (GL_FRONT, GL_DIFFUSE, white);
	}

	sgVec3 sunpos;
	sgSetVec3( sunpos, l->sun_vec[0], l->sun_vec[1], l->sun_vec[2] );
	ssgGetLight( 0 ) -> setPosition( sunpos );

	// xglMatrixMode( GL_PROJECTION );
	// xglLoadIdentity();
 	float fov = current_options.get_fov();
 	ssgSetFOV(fov * current_view.get_win_ratio(), fov);

	double agl = current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER
	    - scenery.cur_elev;

	// FG_LOG( FG_ALL, FG_INFO, "visibility is " 
	//         << current_weather.get_visibility() );
	    
	if ( agl > 10.0 ) {
	    ssgSetNearFar( 10.0f, 100000.0f );
	} else {
	    ssgSetNearFar( 0.5f, 100000.0f );
	}

	if ( current_options.get_view_mode() == 
	     fgOPTIONS::FG_VIEW_PILOT )
        {
	    // disable TuX
	    penguin_sel->select(0);
	} else if ( current_options.get_view_mode() == 
		    fgOPTIONS::FG_VIEW_FOLLOW )
        {
	    // select view matrix from front of view matrix queue
	    // FGMat4Wrapper tmp = current_view.follow.front();
	    // sgCopyMat4( sgVIEW, tmp.m );

	    // enable TuX and set up his position and orientation
	    penguin_sel->select(1);

	    sgMat4 sgTRANS;
	    sgMakeTransMat4( sgTRANS, 
			     pilot_view.view_pos.x(),
			     pilot_view.view_pos.y(),
			     pilot_view.view_pos.z() );

	    sgVec3 ownship_up;
	    sgSetVec3( ownship_up, 0.0, 0.0, 1.0);

	    sgMat4 sgROT;
	    sgMakeRotMat4( sgROT, -90.0, ownship_up );

	    sgMat4 sgTMP;
	    sgMat4 sgTUX;
	    sgMultMat4( sgTMP, sgROT, pilot_view.VIEW_ROT );
	    sgMultMat4( sgTUX, sgTMP, sgTRANS );
	
	    sgCoord tuxpos;
	    sgSetCoord( &tuxpos, sgTUX );
	    penguin_pos->setTransform( &tuxpos );
	}

	ssgSetCamera( current_view.VIEW );

	// position tile nodes and update range selectors
	global_tile_mgr.prep_ssg_nodes();

	// force the default state so ssg can get back on track if
	// we've changed things elsewhere
	FGMaterialSlot m_slot;
	FGMaterialSlot *m_ptr = &m_slot;
	if ( material_mgr.find( "Default", m_ptr ) ) {
	    m_ptr->get_state()->force();
	}

	// draw the ssg scene
	ssgCullAndDraw( scene );

	xglDisable( GL_TEXTURE_2D );
	xglDisable( GL_FOG );

	// display HUD && Panel
	xglDisable( GL_CULL_FACE );
	fgCockpitUpdate();

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
    static fdm_state_list fdm_list;
    // FGInterface fdm_state;
    fgLIGHT *l = &cur_light_params;
    FGTime *t = FGTime::cur_time_params;
    // FGView *v = &current_view;
    int i;

    // update the flight model
    if ( multi_loop < 0 ) {
	multi_loop = 1;
    }

    if ( !t->getPause() ) {
	// run Autopilot system
	fgAPRun();

	// printf("updating flight model x %d\n", multi_loop);
	/* fgFDMUpdate( current_options.get_flight_model(), 
		     fdm_state, 
		     multi_loop * current_options.get_speed_up(),
		     remainder ); */
	cur_fdm_state->update( multi_loop * current_options.get_speed_up() );
    } else {
	// fgFDMUpdate( current_options.get_flight_model(), 
	//              fdm_state, 0, remainder );
	cur_fdm_state->update( 0 );
    }

    fdm_list.push_back( *cur_fdm_state );
    while ( fdm_list.size() > 15 ) {
	fdm_list.pop_front();
    }

    if ( current_options.get_view_mode() == fgOPTIONS::FG_VIEW_PILOT ) {
	cur_view_fdm = *cur_fdm_state;
	// do nothing
    } else if ( current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW ) {
	cur_view_fdm = fdm_list.front();
    }

    // update the view angle
    for ( i = 0; i < multi_loop; i++ ) {
	if ( fabs(current_view.get_goal_view_offset() - 
		  current_view.get_view_offset()) < 0.05 )
	{
	    current_view.set_view_offset( current_view.get_goal_view_offset() );
	    break;
	} else {
	    // move current_view.view_offset towards current_view.goal_view_offset
	    if ( current_view.get_goal_view_offset() > 
		 current_view.get_view_offset() )
            {
		if ( current_view.get_goal_view_offset() - 
		     current_view.get_view_offset() < FG_PI )
                {
		    current_view.inc_view_offset( 0.01 );
		} else {
		    current_view.inc_view_offset( -0.01 );
		}
	    } else {
		if ( current_view.get_view_offset() - 
		     current_view.get_goal_view_offset() < FG_PI )
                {
		    current_view.inc_view_offset( -0.01 );
		} else {
		    current_view.inc_view_offset( 0.01 );
		}
	    }
	    if ( current_view.get_view_offset() > FG_2PI ) {
		current_view.inc_view_offset( -FG_2PI );
	    } else if ( current_view.get_view_offset() < 0 ) {
		current_view.inc_view_offset( FG_2PI );
	    }
	}
    }

    double tmp = -(l->sun_rotation + FG_PI) 
	- (cur_fdm_state->get_Psi() - current_view.get_view_offset() );
    while ( tmp < 0.0 ) {
	tmp += FG_2PI;
    }
    while ( tmp > FG_2PI ) {
	tmp -= FG_2PI;
    }
    /* printf("Psi = %.2f, viewoffset = %.2f sunrot = %.2f rottosun = %.2f\n",
	   FG_Psi * RAD_TO_DEG, current_view.view_offset * RAD_TO_DEG, 
	   -(l->sun_rotation+FG_PI) * RAD_TO_DEG, tmp * RAD_TO_DEG); */
    l->UpdateAdjFog();
}


void fgInitTimeDepCalcs( void ) {
    // initialize timer

    // #ifdef HAVE_SETITIMER
    //   fgTimerInit( 1.0 / current_options.get_model_hz(), 
    //                fgUpdateTimeDepCalcs );
    // #endif HAVE_SETITIMER
}


static const double alt_adjust_ft = 3.758099;
static const double alt_adjust_m = alt_adjust_ft * FEET_TO_METER;


// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void ) {
    FGTime *t;
    static long remainder = 0;
    long elapsed, multi_loop;
#ifdef FANCY_FRAME_COUNTER
    int i;
    double accum;
#else
    static time_t last_time = 0;
    static int frames = 0;
#endif // FANCY_FRAME_COUNTER

    t = FGTime::cur_time_params;

    FG_LOG( FG_ALL, FG_DEBUG, "Running Main Loop");
    FG_LOG( FG_ALL, FG_DEBUG, "======= ==== ====");

#if defined( ENABLE_PLIB_JOYSTICK )
    // Read joystick and update control settings
    if ( current_options.get_control_mode() == fgOPTIONS::FG_JOYSTICK ) {
	fgJoystickRead();
    }
#elif defined( ENABLE_GLUT_JOYSTICK )
    // Glut joystick support works by feeding a joystick handler
    // function to glut.  This is taken care of once in the joystick
    // init routine and we don't have to worry about it again.
#endif

#ifdef FG_OLD_WEATHER
    current_weather.Update();
#endif

    // Fix elevation.  I'm just sticking this here for now, it should
    // probably move eventually

    /* printf("Before - ground = %.2f  runway = %.2f  alt = %.2f\n",
	   scenery.cur_elev,
	   cur_fdm_state->get_Runway_altitude() * FEET_TO_METER,
	   cur_fdm_state->get_Altitude() * FEET_TO_METER); */

    if ( scenery.cur_elev > -9990 ) {
	if ( cur_fdm_state->get_Altitude() * FEET_TO_METER < 
	     (scenery.cur_elev + alt_adjust_m - 3.0) ) {
	    // now set aircraft altitude above ground
	    printf("Current Altitude = %.2f < %.2f forcing to %.2f\n", 
		   cur_fdm_state->get_Altitude() * FEET_TO_METER,
		   scenery.cur_elev + alt_adjust_m - 3.0,
		   scenery.cur_elev + alt_adjust_m );
	    fgFDMForceAltitude( current_options.get_flight_model(), 
				scenery.cur_elev + alt_adjust_m );

	    FG_LOG( FG_ALL, FG_DEBUG, 
		    "<*> resetting altitude to " 
		    << cur_fdm_state->get_Altitude() * FEET_TO_METER << " meters" );
	}
	fgFDMSetGroundElevation( current_options.get_flight_model(),
				 scenery.cur_elev );  // meters
    }

    /* printf("Adjustment - ground = %.2f  runway = %.2f  alt = %.2f\n",
	   scenery.cur_elev,
	   cur_fdm_state->get_Runway_altitude() * FEET_TO_METER,
	   cur_fdm_state->get_Altitude() * FEET_TO_METER); */

    // update "time"
    t->update(*cur_fdm_state);

    // Get elapsed time (in usec) for this past frame
    elapsed = fgGetTimeInterval();
    FG_LOG( FG_ALL, FG_DEBUG, 
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
	FG_LOG( FG_ALL, FG_DEBUG, 
		"--> Frame rate is = " << general.get_frame_rate() );
	frames = 0;
    }
    last_time = t->get_cur_time();
    ++frames;
#endif

    // Run flight model
    if ( ! use_signals ) {
	// Calculate model iterations needed for next frame
	elapsed += remainder;

	multi_loop = (int)(((double)elapsed * 0.000001) * 
			   current_options.get_model_hz());
	remainder = elapsed - ( (multi_loop*1000000) / 
				current_options.get_model_hz() );
	FG_LOG( FG_ALL, FG_DEBUG, 
		"Model iterations needed = " << multi_loop
		<< ", new remainder = " << remainder );
	
	// flight model
	if ( multi_loop > 0 ) {
	    fgUpdateTimeDepCalcs(multi_loop, remainder);
	} else {
	    FG_LOG( FG_ALL, FG_DEBUG, 
		    "Elapsed time is zero ... we're zinging" );
	}
    }

#if ! defined( MACOS )
    // Do any serial port work that might need to be done
    fgSerialProcess();
#endif

    // see if we need to load any new scenery tiles
    global_tile_mgr.update();

    // Process/manage pending events
    global_events.Process();

    // Run audio scheduler
#ifdef ENABLE_AUDIO_SUPPORT
    if ( current_options.get_sound() && !audio_sched->not_working() ) {

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
	if (controls.get_throttle(0) > 0.0 || cur_fdm_state->v_rel_wind > 40.0) {
	    //fprintf(stderr, "rel_wind: %f ", cur_fdm_state->v_rel_wind);
	    // only add relative wind and AoA if prop is moving
	    // or we're really flying at idle throttle
	    if (pitch < 5.4) {  // this needs tuning
		// prop tips not breaking sound barrier
		pitch += log(cur_fdm_state->v_rel_wind + 0.8)/2;
	    } else {
		// prop tips breaking sound barrier
		pitch += log(cur_fdm_state->v_rel_wind + 0.8)/10;
	    }
	    //fprintf(stderr, "pitch2: %f ", pitch);
	    //fprintf(stderr, "AoA: %f ", FG_Gamma_vert_rad);

	    // Angle of Attack next... -x^3(e^x) is my best guess Just
	    // need to calculate some reasonable scaling factor and
	    // then clamp it on the positive aoa (neg adj) side
	    double aoa = cur_fdm_state->get_Gamma_vert_rad() * 2.2;
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
	    log(cur_fdm_state->v_rel_wind + 1.0)/14.0;
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
	    FGPath mp3file( current_options.get_fg_root() );
	    mp3file.append( "Sounds/intro.mp3" );

	    string command = "(touch " + lockfile + "; mpg123 "
		+ mp3file.str() + "> /dev/null 2>&1; /bin/rm "
		+ lockfile + ") &";
	    FG_LOG( FG_GENERAL, FG_INFO, 
		    "Starting intro music: " << mp3file.str() );
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
	audio_mixer -> setMasterVolume ( 80 ) ;  /* 80% of max volume. */
	audio_sched -> setSafetyMargin ( 1.0 ) ;

	FGPath slfile( current_options.get_fg_root() );
	slfile.append( "Sounds/wasp.wav" );

	s1 = new slSample ( (char *)slfile.c_str() );
	FG_LOG( FG_GENERAL, FG_INFO,
		"Rate = " << s1 -> getRate()
		<< "  Bps = " << s1 -> getBps()
		<< "  Stereo = " << s1 -> getStereo() );
	audio_sched -> loopSample ( s1 );

	if ( audio_sched->not_working() ) {
	    // skip
	} else {
	    pitch_envelope.setStep  ( 0, 0.01, 0.6 );
	    volume_envelope.setStep ( 0, 0.01, 0.6 );

	    audio_sched -> addSampleEnvelope( s1, 0, 0, 
					      &pitch_envelope,
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

// options.cxx needs to see this for toggle_panel()
// Handle new window size or exposure
void fgReshape( int width, int height ) {
    if ( ! current_options.get_panel_status() ) {
	current_view.set_win_ratio( (GLfloat) width / (GLfloat) height );
	xglViewport(0, 0 , (GLint)(width), (GLint)(height) );
    } else {
	current_view.set_win_ratio( (GLfloat) width / 
	                            ((GLfloat) (height)*0.4232) );
	xglViewport(0, (GLint)((height)*0.5768), (GLint)(width), 
		    (GLint)((height)*0.4232) );
    }

    current_view.set_winWidth( width );
    current_view.set_winHeight( height );
    current_view.force_update_fov_math();

    // set these fov to be the same as in fgRenderFrame()
    float x_fov = current_options.get_fov();
    float y_fov = x_fov * 1.0 / current_view.get_win_ratio();
    ssgSetFOV( x_fov, y_fov );

    glViewport ( 0, 0, width, height );
    float fov = current_options.get_fov();
    ssgSetFOV(fov * current_view.get_win_ratio(), fov);

    if ( idle_state == 1000 ) {
	// yes we've finished all our initializations and are running
	// the main loop, so this will now work without seg faulting
	// the system.
	solarSystemRebuild();
	current_view.UpdateViewParams(cur_view_fdm);
	if ( current_options.get_panel_status() ) {
	    FGPanel::OurPanel->ReInit(0, 0, 1024, 768);
	}
    }
}


// Initialize GLUT and define a main window
int fgGlutInit( int *argc, char **argv ) {

#if !defined( MACOS )
    // GLUT will extract all glut specific options so later on we only
    // need wory about our own.
    xglutInit(argc, argv);
#endif

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
#ifndef GLUT_WRONG_VERSION
    } else {
	// Open the cool new 'game mode' window
	char game_mode_str[256];
	sprintf( game_mode_str, "width=%d height=%d bpp=32",
		 current_options.get_xsize(),
		 current_options.get_ysize() );

	FG_LOG( FG_GENERAL, FG_INFO, 
		"game mode params = " << game_mode_str );
	glutGameModeString( game_mode_str );
	glutEnterGameMode();
#endif
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

#if defined( MACOS )
    argc = ccommand( &argv );
#endif

#ifdef HAVE_BC5PLUS
    _control87(MCW_EM, MCW_EM);  /* defined in float.h */
#endif

    // set default log levels
    fglog().setLogLevels( FG_ALL, FG_INFO );

    FG_LOG( FG_GENERAL, FG_INFO, "Flight Gear:  Version " << VERSION << endl );

    // seed the random number generater
    fg_srandom();

    // Load the configuration parameters
    if ( !fgInitConfig(argc, argv) ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Config option parsing failed ..." );
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

    // Initialize ssg (from plib)
    ssgInit();

    // Initialize the user interface (we need to do this before
    // passing off control to glut and before fgInitGeneral to get our
    // fonts !!!
    guiInit();

    // Do some quick general initializations
    if( !fgInitGeneral()) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"General initializations failed ..." );
	exit(-1);
    }

    //
    // some ssg test stuff (requires data from the plib source
    // distribution) specifically from the ssg tux example
    //

    FGPath modelpath( current_options.get_fg_root() );
    modelpath.append( "Models" );
    modelpath.append( "Geometry" );
  
    FGPath texturepath( current_options.get_fg_root() );
    texturepath.append( "Models" );
    texturepath.append( "Textures" );
  
    ssgModelPath( (char *)modelpath.c_str() );
    ssgTexturePath( (char *)texturepath.c_str() );

    scene = new ssgRoot;
    terrain = new ssgBranch;
    terrain->setName( "Terrain" );
    penguin_sel = new ssgSelector;
    penguin_pos = new ssgTransform;

    ssgEntity *tux_obj = ssgLoadAC( "glider.ac" );
    // ssgEntity *tux_obj = ssgLoadAC( "Tower1x.ac" );
    penguin_pos->addKid( tux_obj );
    penguin_sel->addKid( penguin_pos );
    ssgFlatten( tux_obj );
    ssgStripify( penguin_sel );

#ifdef FG_NETWORK_OLK
    // Do the network intialization
    printf("Multipilot mode %s\n", fg_net_init() );
#endif

    scene->addKid( terrain );
    scene->addKid( penguin_sel );

    // pass control off to the master GLUT event handler
    glutMainLoop();

    // we never actually get here ... but to avoid compiler warnings,
    // etc.
    return 0;
}
