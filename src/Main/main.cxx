// main.cxx -- top level sim routines
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997 - 2002  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined(__linux__) && defined(__i386__)
#  include <fpu_control.h>
#  include <signal.h>
#endif

#include <simgear/compiler.h>

#include STL_IOSTREAM
#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(cerr);
SG_USING_STD(endl);
#endif

#include <simgear/misc/exception.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/route/route.hxx>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#  include <float.h>
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <stdio.h>
#include <string.h>		// for strcmp()
#include <string>

#ifdef HAVE_STDLIB_H
#   include <stdlib.h>
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>		// for stat()
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>		// for stat()
#endif

#include <plib/netChat.h>
#include <plib/pu.h>
#include <plib/ssg.h>

#include <simgear/constants.h>  // for VERSION
#include <simgear/debug/logstream.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sky/sky.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Include/general.hxx>

#include <Aircraft/aircraft.hxx>

#include <ATC/ATCmgr.hxx>
#include <ATC/ATCdisplay.hxx>
#include <ATC/AIMgr.hxx>

#include <Autopilot/newauto.hxx>

#include <Cockpit/cockpit.hxx>
#include <Cockpit/radiostack.hxx>
#include <Cockpit/steam.hxx>

#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <GUI/gui.h>
#include <Model/acmodel.hxx>
#include <Model/modelmgr.hxx>
#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif
#include <Objects/matlib.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#ifdef ENABLE_AUDIO_SUPPORT
#  include <Sound/soundmgr.hxx>
#  include <Sound/fg_fx.hxx>
#  include <Sound/morse.hxx>
#endif
#include <Time/FGEventMgr.hxx>
#include <Time/fg_timer.hxx>
#include <Time/light.hxx>
#include <Time/sunpos.hxx>
#include <Time/tmp.hxx>

#include <Input/input.hxx>

// ADA
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/point3d.hxx>
#include <FDM/flight.hxx>
#include <FDM/ADA.hxx>
#include <Scenery/tileentry.hxx>

#if !defined(sgi)
// PFNGLPOINTPARAMETERFEXTPROC glPointParameterfEXT = 0;
// PFNGLPOINTPARAMETERFVEXTPROC glPointParameterfvEXT = 0;
#endif
float default_attenuation[3] = {1.0, 0.0, 0.0};
//Required for using GL_extensions
void fgLoadDCS (void);
void fgUpdateDCS (void);
ssgSelector *ship_sel=NULL;
// upto 32 instances of a same object can be loaded.
ssgTransform *ship_pos[32];
double obj_lat[32],obj_lon[32],obj_alt[32],obj_pitch[32],obj_roll[32];
int objc=0;
ssgSelector *lightpoints_brightness = new ssgSelector;
ssgTransform *lightpoints_transform = new ssgTransform;
FGTileEntry *dummy_tile;
sgVec3 rway_ols;
// ADA
// Clip plane settings...
float scene_nearplane = 0.5f;
float scene_farplane = 120000.0f;


#ifndef FG_NEW_ENVIRONMENT
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Environment/environment_mgr.hxx>
#endif

#include "version.h"

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "splash.hxx"
#include "viewmgr.hxx"
#include "options.hxx"
#include "logger.hxx"

#ifdef macintosh
#  include <console.h>		// -dw- for command line dialog
#endif


FGEventMgr global_events;

// This is a record containing a bit of global housekeeping information
FGGeneral general;

// Specify our current idle function state.  This is used to run all
// our initializations out of the glutIdleLoop() so that we can get a
// splash screen up and running right away.
static int idle_state = 0;
static long global_multi_loop;

// forward declaration
void fgReshape( int width, int height );

// fog constants.  I'm a little nervous about putting actual code out
// here but it seems to work (?)
static const double m_log01 = -log( 0.01 );
static const double sqrt_m_log01 = sqrt( m_log01 );
static GLfloat fog_exp_density;
static GLfloat fog_exp2_density;
static GLfloat fog_exp2_punch_through;

ssgRoot *lighting = NULL;
// ssgBranch *airport = NULL;

#ifdef FG_NETWORK_OLK
ssgSelector *fgd_sel = NULL;
ssgTransform *fgd_pos = NULL;
#endif

// current fdm/position used for view
FGInterface cur_view_fdm;

// Sky structures
SGSky *thesky;

// hack
sgMat4 copy_of_ssgOpenGLAxisSwapMatrix =
{
  {  1.0f,  0.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f, -1.0f,  0.0f },
  {  0.0f,  1.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f,  0.0f,  1.0f }
};

// The following defines flightgear options. Because glutlib will also
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


ssgSimpleState *default_state;
ssgSimpleState *hud_and_panel;
ssgSimpleState *menus;

SGTimeStamp start_time_stamp;
SGTimeStamp current_time_stamp;

void fgBuildRenderStates( void ) {
    default_state = new ssgSimpleState;
    default_state->ref();
    default_state->disable( GL_TEXTURE_2D );
    default_state->enable( GL_CULL_FACE );
    default_state->enable( GL_COLOR_MATERIAL );
    default_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    default_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    default_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    default_state->disable( GL_BLEND );
    default_state->disable( GL_ALPHA_TEST );
    default_state->disable( GL_LIGHTING );

    hud_and_panel = new ssgSimpleState;
    hud_and_panel->ref();
    hud_and_panel->disable( GL_CULL_FACE );
    hud_and_panel->disable( GL_TEXTURE_2D );
    hud_and_panel->disable( GL_LIGHTING );
    hud_and_panel->enable( GL_BLEND );

    menus = new ssgSimpleState;
    menus->ref();
    menus->disable( GL_CULL_FACE );
    menus->disable( GL_TEXTURE_2D );
    menus->enable( GL_BLEND );
}


// fgInitVisuals() -- Initialize various GL/view parameters
void fgInitVisuals( void ) {
    fgLIGHT *l;

    l = &cur_light_params;

#ifndef GLUT_WRONG_VERSION
    // Go full screen if requested ...
    if ( fgGetBool("/sim/startup/fullscreen") ) {
	glutFullScreen();
    }
#endif

    // If enabled, normal vectors specified with glNormal are scaled
    // to unit length after transformation.  Enabling this has
    // performance implications.  See the docs for glNormal.
    // glEnable( GL_NORMALIZE );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    glLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

    sgVec3 sunpos;
    sgSetVec3( sunpos, l->sun_vec[0], l->sun_vec[1], l->sun_vec[2] );
    ssgGetLight( 0 ) -> setPosition( sunpos );

    glFogi (GL_FOG_MODE, GL_EXP2);
    if ( (!strcmp(fgGetString("/sim/rendering/fog"), "disabled")) || 
	 (!fgGetBool("/sim/rendering/shading"))) {
	// if fastest fog requested, or if flat shading force fastest
	glHint ( GL_FOG_HINT, GL_FASTEST );
    } else if ( !strcmp(fgGetString("/sim/rendering/fog"), "nicest") ) {
	glHint ( GL_FOG_HINT, GL_NICEST );
    }
    if ( fgGetBool("/sim/rendering/wireframe") ) {
	// draw wire frame
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    // This is the default anyways, but it can't hurt
    glFrontFace ( GL_CCW );

    // Just testing ...
    // glEnable(GL_POINT_SMOOTH);
    // glEnable(GL_LINE_SMOOTH);
    // glEnable(GL_POLYGON_SMOOTH);      
}


// For HiRes screen Dumps using Brian Pauls TR Library
void trRenderFrame( void ) {

    if ( fgPanelVisible() ) {
        GLfloat height = fgGetInt("/sim/startup/ysize");
        GLfloat view_h =
            (current_panel->getViewHeight() - current_panel->getYOffset())
            * (height / 768.0) + 1;
        glTranslatef( 0.0, view_h, 0.0 );
    }

    static GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    static GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

    fgLIGHT *l = &cur_light_params;

    glClearColor(l->adj_fog_color[0], l->adj_fog_color[1], 
                 l->adj_fog_color[2], l->adj_fog_color[3]);

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // set the opengl state to known default values
    default_state->force();

    glEnable( GL_FOG );
    glFogf  ( GL_FOG_DENSITY, fog_exp2_density);
    glFogi  ( GL_FOG_MODE,    GL_EXP2 );
    glFogfv ( GL_FOG_COLOR,   l->adj_fog_color );

    // GL_LIGHT_MODEL_AMBIENT has a default non-zero value so if
    // we only update GL_AMBIENT for our lights we will never get
    // a completely dark scene.  So, we set GL_LIGHT_MODEL_AMBIENT
    // explicitely to black.
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, black );

    ssgGetLight( 0 ) -> setColour( GL_AMBIENT, l->scene_ambient );

    // texture parameters
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;

    // we need a white diffuse light for the phase of the moon
    ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, white );
    thesky->preDraw();

    // draw the ssg scene
    // return to the desired diffuse color
    ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse );
    glEnable( GL_DEPTH_TEST );
    ssgSetNearFar( scene_nearplane, scene_farplane );
    ssgCullAndDraw( globals->get_scene_graph() );

    // draw the lights
    glFogf (GL_FOG_DENSITY, fog_exp2_punch_through);
    ssgSetNearFar( scene_nearplane, scene_farplane );
    ssgCullAndDraw( lighting );

    thesky->postDraw( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER );

    globals->get_model_mgr()->draw();
    globals->get_aircraft_model()->draw();

    // need to do this here as hud_and_panel state is static to
    // main.cxx  and HUD and Panel routines have to be called with
    // knowledge of the the TR struct < see gui.cxx::HighResDump()
    hud_and_panel->apply();
}


// Update all Visuals (redraws anything graphics related)
void fgRenderFrame( void ) {
  
    static long old_elapsed_ms = 0;

    int dt_ms = int(globals->get_elapsed_time_ms() - old_elapsed_ms);
    old_elapsed_ms = globals->get_elapsed_time_ms();

    // Process/manage pending events
    global_events.update( dt_ms );

    static const SGPropertyNode *longitude
	= fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
	= fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *altitude
	= fgGetNode("/position/altitude-ft");

    // Update the default (kludged) properties.
    fgUpdateProps();

    FGViewer *current__view = globals->get_current_view();

    fgLIGHT *l = &cur_light_params;
    static double last_visibility = -9999;

    // update fog params
    double actual_visibility       = thesky->get_visibility();
    if ( actual_visibility != last_visibility ) {
        last_visibility = actual_visibility;

        fog_exp_density = m_log01 / actual_visibility;
        fog_exp2_density = sqrt_m_log01 / actual_visibility;
        fog_exp2_punch_through = sqrt_m_log01 / ( actual_visibility * 2.5 );
    }

    // double angle;
    // GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    // GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    // GLfloat terrain_color[4] = { 0.54, 0.44, 0.29, 1.0 };
    // GLfloat mat_shininess[] = { 10.0 };
    GLbitfield clear_mask;
    
    if ( idle_state != 1000 ) {
	// still initializing, draw the splash screen
	if ( fgGetBool("/sim/startup/splash-screen") ) {
	    fgSplashUpdate(0.0);
	}
	start_time_stamp.stamp(); // Keep resetting the start time
    } else {
	// idle_state is now 1000 meaning we've finished all our
	// initializations and are running the main loop, so this will
	// now work without seg faulting the system.

        // Update the elapsed time.
        current_time_stamp.stamp();
	globals->set_elapsed_time_ms( (current_time_stamp - start_time_stamp)
                                      / 1000L );

	// calculate our current position in cartesian space
	scenery.set_center( scenery.get_next_center() );

	// update view port
	fgReshape( fgGetInt("/sim/startup/xsize"),
		   fgGetInt("/sim/startup/ysize") );

	// set the sun position
	glLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

	clear_mask = GL_DEPTH_BUFFER_BIT;
	if ( fgGetBool("/sim/rendering/wireframe") ) {
	    clear_mask |= GL_COLOR_BUFFER_BIT;
	}

	if ( fgGetBool("/sim/rendering/skyblend") ) {
	    if ( fgGetBool("/sim/rendering/textures") ) {
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
	glClear( clear_mask );

	// Tell GL we are switching to model view parameters

	// I really should create a derived ssg node or use a call
	// back or something so that I can draw the sky within the
	// ssgCullAndDraw() function, but for now I just mimic what
	// ssg does to set up the model view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ssgSetCamera( (sgVec4 *)current__view->get_VIEW() );

	// set the opengl state to known default values
	default_state->force();

	// update fog params if visibility has changed
	double visibility_meters = fgGetDouble("/environment/visibility-m");
	thesky->set_visibility(visibility_meters);

	thesky->modify_vis( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER,
			    ( global_multi_loop * fgGetInt("/sim/speed-up") )
                            / (double)fgGetInt("/sim/model-hz") );

	// Set correct opengl fog density
	glFogf (GL_FOG_DENSITY, fog_exp2_density);

	// update the sky dome
	if ( fgGetBool("/sim/rendering/skyblend") ) {
	    /* cout << "thesky->repaint() sky_color = "
		 << cur_light_params.sky_color[0] << " "
		 << cur_light_params.sky_color[1] << " "
		 << cur_light_params.sky_color[2] << " "
		 << cur_light_params.sky_color[3] << endl;
	    cout << "    fog = "
		 << cur_light_params.fog_color[0] << " "
		 << cur_light_params.fog_color[1] << " "
		 << cur_light_params.fog_color[2] << " "
		 << cur_light_params.fog_color[3] << endl;
	    cout << "    sun_angle = " << cur_light_params.sun_angle
		 << "    moon_angle = " << cur_light_params.moon_angle
                 << endl; */
	    thesky->repaint( cur_light_params.sky_color,
			     cur_light_params.adj_fog_color,
			     cur_light_params.sun_angle,
			     cur_light_params.moon_angle,
			     globals->get_ephem()->getNumPlanets(),
			     globals->get_ephem()->getPlanets(),
			     globals->get_ephem()->getNumStars(),
			     globals->get_ephem()->getStars() );
 
	    /* cout << "thesky->reposition( view_pos = " << view_pos[0] << " "
		 << view_pos[1] << " " << view_pos[2] << endl;
	    cout << "    zero_elev = " << zero_elev[0] << " "
		 << zero_elev[1] << " " << zero_elev[2]
		 << " lon = " << cur_fdm_state->get_Longitude()
		 << " lat = " << cur_fdm_state->get_Latitude() << endl;
	    cout << "    sun_rot = " << cur_light_params.sun_rotation
		 << " gst = " << SGTime::cur_time_params->getGst() << endl;
	    cout << "    sun ra = " << globals->get_ephem()->getSunRightAscension()
		 << " sun dec = " << globals->get_ephem()->getSunDeclination() 
		 << " moon ra = " << globals->get_ephem()->getMoonRightAscension()
		 << " moon dec = " << globals->get_ephem()->getMoonDeclination() << endl; */

	    thesky->reposition( current__view->get_view_pos(),
				current__view->get_zero_elev(),
				current__view->get_world_up(),
				longitude->getDoubleValue()
				  * SGD_DEGREES_TO_RADIANS,
				latitude->getDoubleValue()
				  * SGD_DEGREES_TO_RADIANS,
				altitude->getDoubleValue() * SG_FEET_TO_METER,
				cur_light_params.sun_rotation,
				globals->get_time_params()->getGst(),
				globals->get_ephem()->getSunRightAscension(),
				globals->get_ephem()->getSunDeclination(),
				50000.0,
				globals->get_ephem()->getMoonRightAscension(),
				globals->get_ephem()->getMoonDeclination(),
				50000.0 );
	}

	glEnable( GL_DEPTH_TEST );
	if ( strcmp(fgGetString("/sim/rendering/fog"), "disabled") ) {
	    glEnable( GL_FOG );
	    glFogi( GL_FOG_MODE, GL_EXP2 );
	    glFogfv( GL_FOG_COLOR, l->adj_fog_color );
	}

	// set sun/lighting parameters
	ssgGetLight( 0 ) -> setPosition( l->sun_vec );

        // GL_LIGHT_MODEL_AMBIENT has a default non-zero value so if
        // we only update GL_AMBIENT for our lights we will never get
        // a completely dark scene.  So, we set GL_LIGHT_MODEL_AMBIENT
        // explicitely to black.
	GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, black );

	ssgGetLight( 0 ) -> setColour( GL_AMBIENT, l->scene_ambient );
	ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse );
	// ssgGetLight( 0 ) -> setColour( GL_SPECULAR, l->scene_white );

	// texture parameters
	// glEnable( GL_TEXTURE_2D );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;

	// glMatrixMode( GL_PROJECTION );
	// glLoadIdentity();
	ssgSetFOV( current__view->get_h_fov(),
		   current__view->get_v_fov() );

	double agl = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
	    - scenery.get_cur_elev();

//	if (fgGetBool("/sim/view/internal"))
//	  ssgSetNearFar( 0.2f, 120000.0f );
//	else if ( agl > 10.0)

        if ( agl > 10.0 ) {
            scene_nearplane = 10.0f;
            scene_farplane = 120000.0f;
	} else {
            scene_nearplane = 0.5f;
            scene_farplane = 120000.0f;
        }

        ssgSetNearFar( scene_nearplane, scene_farplane );

	// $$$ begin - added VS Renganthan 17 Oct 2K
	if(objc)
	  fgUpdateDCS();
	// $$$ end - added VS Renganthan 17 Oct 2K

# ifdef FG_NETWORK_OLK
	if ( fgGetBool("/sim/networking/network-olk") ) {
	    sgCoord fgdpos;
	    other = head->next;             /* put listpointer to start  */
	    while ( other != tail) {        /* display all except myself */
		if ( strcmp( other->ipadr, fgd_mcp_ip) != 0) {
		    other->fgd_sel->select(1);
		    sgSetCoord( &fgdpos, other->sgFGD_COORD );
		    other->fgd_pos->setTransform( &fgdpos );
		}
		other = other->next;
	    }

	    // fgd_sel->select(1);
	    // sgCopyMat4( sgTUX, current_view.sgVIEW);
	    // sgCoord fgdpos;
	    // sgSetCoord( &fgdpos, sgFGD_VIEW );
	    // fgd_pos->setTransform( &fgdpos);
	}
# endif

	// position tile nodes and update range selectors
	global_tile_mgr.prep_ssg_nodes(visibility_meters);

	if ( fgGetBool("/sim/rendering/skyblend") ) {
	    // draw the sky backdrop

            // we need a white diffuse light for the phase of the moon
            ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, white );

	    thesky->preDraw();

            // return to the desired diffuse color
            ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse );
	}

	// draw the ssg scene
	glEnable( GL_DEPTH_TEST );

        ssgSetNearFar( scene_nearplane, scene_farplane );
	ssgCullAndDraw( globals->get_scene_graph() );

	// change state for lighting here

	// draw lighting
	// Set punch through fog density
	glFogf (GL_FOG_DENSITY, fog_exp2_punch_through);

#ifdef FG_EXPERIMENTAL_LIGHTING
        // Enable states for drawing points with GL_extension
	if (glutExtensionSupported("GL_EXT_point_parameters")) {
            glEnable(GL_POINT_SMOOTH);
            float quadratic[3] = {1.0, 0.01, 0.0001};
            // get the address of our OpenGL extensions
#if !defined(sgi)
            glPointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC) 
                wglGetProcAddress("glPointParameterfEXT");
            glPointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC) 
                wglGetProcAddress("glPointParameterfvEXT");
#endif
            // makes the points fade as they move away
            glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, quadratic);
            glPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, 1.0); 
            glPointSize(4.0);

            // Enable states for drawing runway lights with spherical mapping
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);

            //Maybe this is not the best way, but it works !!
            glPolygonMode(GL_FRONT, GL_POINT);
            glCullFace(GL_BACK);
            glEnable(GL_CULL_FACE);
	}

	glDisable( GL_LIGHTING );
        // blending function for runway lights
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
	glEnable(GL_BLEND);
#endif

        ssgSetNearFar( scene_nearplane, scene_farplane );
	ssgCullAndDraw( lighting );


#ifdef FG_EXPERIMENTAL_LIGHTING
	if (glutExtensionSupported("GL_EXT_point_parameters")) {
            // Disable states used for runway lighting
            glPolygonMode(GL_FRONT, GL_FILL);

            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT,
                                  default_attenuation);
	}

        glPointSize(1.0);
#endif

	if ( fgGetBool("/sim/rendering/skyblend") ) {
	    // draw the sky cloud layers
	    thesky->postDraw( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER );
	}

	globals->get_model_mgr()->draw();
	globals->get_aircraft_model()->draw();

	// display HUD && Panel
	glDisable( GL_FOG );
	glDisable( GL_DEPTH_TEST );
	// glDisable( GL_CULL_FACE );
	// glDisable( GL_TEXTURE_2D );

	// update the input subsystem
	current_input.update(dt_ms);

	// update the controls subsystem
	globals->get_controls()->update(dt_ms);

	hud_and_panel->apply();
	fgCockpitUpdate();

	// Use the hud_and_panel ssgSimpleState for rendering the ATC output
	// This only works properly if called before the panel call
	globals->get_ATC_display()->update(dt_ms);

	// update the panel subsystem
	if ( current_panel != NULL ) {
	    current_panel->update(dt_ms);
	}

	// We can do translucent menus, so why not. :-)
	menus->apply();
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	puDisplay();
	// glDisable ( GL_BLEND ) ;

	// glEnable( GL_FOG );

	globals->get_logger()->update(dt_ms);
    }

    glutSwapBuffers();
}


// Update internal time dependent calculations (i.e. flight model)
void fgUpdateTimeDepCalcs() {
    static bool inited = false;
    static const SGPropertyNode *master_freeze
	= fgGetNode("/sim/freeze/master");

    //SG_LOG(SG_FLIGHT,SG_INFO, "Updating time dep calcs()");

    fgLIGHT *l = &cur_light_params;
    int i;

    long multi_loop = 1;

    // Initialize the FDM here if it hasn't been and if we have a
    // scenery elevation hit.

    // cout << "cur_fdm_state->get_inited() = " << cur_fdm_state->get_inited() 
    //      << " cur_elev = " << scenery.get_cur_elev() << endl;

    if ( !cur_fdm_state->get_inited() && scenery.get_cur_elev() > -9990 ) {
	SG_LOG(SG_FLIGHT,SG_INFO, "Finally initializing fdm");  
	
	cur_fdm_state->init();
	if ( cur_fdm_state->get_bound() ) {
	    cur_fdm_state->unbind();
	}
	cur_fdm_state->bind();
    }

    // conceptually, the following block could be done for each fdm
    // instance ...
    if ( !cur_fdm_state->get_inited() ) {
	// do nothing, fdm isn't inited yet
    } else if ( master_freeze->getBoolValue() ) {
	// we are frozen, run the fdm's with 0 time slices in case
	// they want to do something with that.

	cur_fdm_state->update( 0 );
	FGSteam::update( 0 );
    } else {
	// we have been inited, and we are not frozen, we are good to go ...

	if ( !inited ) {
	    cur_fdm_state->stamp();
	    inited = true;
	}

	SGTimeStamp current;
	current.stamp();
	long elapsed = current - cur_fdm_state->get_time_stamp();
	cur_fdm_state->set_time_stamp( current );
	elapsed += cur_fdm_state->get_remainder();
	// cout << "elapsed = " << elapsed << endl;
	// cout << "dt = " << cur_fdm_state->get_delta_t() << endl;
	multi_loop = (long)(((double)elapsed * 0.000001) /
			       cur_fdm_state->get_delta_t() );
	cur_fdm_state->set_multi_loop( multi_loop );
	long remainder = elapsed - (long)( (multi_loop*1000000) *
				     cur_fdm_state->get_delta_t() );
	cur_fdm_state->set_remainder( remainder );
	// cout << "remainder = " << remainder << endl;

	// chop max interations to something reasonable if the sim was
	// delayed for an excesive amount of time
	if ( multi_loop > 2.0 / cur_fdm_state->get_delta_t() ) {
	    multi_loop = (int)(2.0 / cur_fdm_state->get_delta_t());
	    cur_fdm_state->set_remainder( 0 );
	}

	// cout << "multi_loop = " << multi_loop << endl;
	for ( i = 0; i < multi_loop * fgGetInt("/sim/speed-up"); ++i ) {
	    // run Autopilot system
	    globals->get_autopilot()->update(0); // FIXME: use real dt

	    // update FDM
	    cur_fdm_state->update( 1 );
	}
	FGSteam::update( multi_loop * fgGetInt("/sim/speed-up") );
    }

    if ( !strcmp(fgGetString("/sim/view-mode"), "pilot") ) {
	cur_view_fdm = *cur_fdm_state;
	// do nothing
    }


    globals->get_model_mgr()->update(multi_loop);
    globals->get_aircraft_model()->update(multi_loop);

    // update the view angle
    globals->get_viewmgr()->update(multi_loop);

    l->UpdateAdjFog();

    // Update solar system
    globals->get_ephem()->update( globals->get_time_params()->getMjd(),
				  globals->get_time_params()->getLst(),
				  cur_fdm_state->get_Latitude() );

    // Update radio stack model
    current_radiostack->update(multi_loop);
}


void fgInitTimeDepCalcs( void ) {
    // noop for now
}


static const double alt_adjust_ft = 3.758099;
static const double alt_adjust_m = alt_adjust_ft * SG_FEET_TO_METER;


// What should we do when we have nothing else to do?  Let's get ready
// for the next move and update the display?
static void fgMainLoop( void ) {
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

    SG_LOG( SG_ALL, SG_DEBUG, "Running Main Loop");
    SG_LOG( SG_ALL, SG_DEBUG, "======= ==== ====");

#ifdef FG_NETWORK_OLK
    if ( fgGetBool("/sim/networking/network-olk") ) {
	if ( net_is_registered == 0 ) {	     // We first have to reg. to fgd
	    // printf("FGD: Netupdate\n");
	    fgd_send_com( "A", FGFS_host);   // Send Mat4 data
	    fgd_send_com( "B", FGFS_host);   // Recv Mat4 data
	}
    }
#endif

#if defined( ENABLE_PLIB_JOYSTICK )
    // Read joystick and update control settings
    // if ( fgGetString("/sim/control-mode") == "joystick" )
    // {
    //    fgJoystickRead();
    // }
#elif defined( ENABLE_GLUT_JOYSTICK )
    // Glut joystick support works by feeding a joystick handler
    // function to glut.  This is taken care of once in the joystick
    // init routine and we don't have to worry about it again.
#endif

#ifdef FG_NEW_ENVIRONMENT
    globals->get_environment_mgr()->update(0);	// FIXME: use real delta time
#endif

    // Fix elevation.  I'm just sticking this here for now, it should
    // probably move eventually

    /* printf("Before - ground = %.2f  runway = %.2f  alt = %.2f\n",
	   scenery.get_cur_elev(),
	   cur_fdm_state->get_Runway_altitude() * SG_FEET_TO_METER,
	   cur_fdm_state->get_Altitude() * SG_FEET_TO_METER); */

    if ( scenery.get_cur_elev() > -9990 && cur_fdm_state->get_inited() ) {
	if ( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER < 
	     (scenery.get_cur_elev() + alt_adjust_m - 3.0) ) {
	    // now set aircraft altitude above ground
	    printf("(*) Current Altitude = %.2f < %.2f forcing to %.2f\n", 
		   cur_fdm_state->get_Altitude() * SG_FEET_TO_METER,
		   scenery.get_cur_elev() + alt_adjust_m - 3.0,
		   scenery.get_cur_elev() + alt_adjust_m );
	    cur_fdm_state->set_Altitude( scenery.get_cur_elev() 
                                                + alt_adjust_m );

	    SG_LOG( SG_ALL, SG_DEBUG, 
		    "<*> resetting altitude to " 
		    << cur_fdm_state->get_Altitude() * SG_FEET_TO_METER
		    << " meters" );
	}
    }

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

    if ( globals->get_warp_delta() != 0 ) {
	fgUpdateSkyAndLightingParams();
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
	SG_LOG( SG_ALL, SG_DEBUG, 
		"--> Frame rate is = " << general.get_frame_rate() );
	frames = 0;
    }
    last_time = t->get_cur_time();
    ++frames;
#endif

    // Run ATC subsystem
    globals->get_ATC_mgr()->update(1);	// FIXME - use real dt.

    // Run the AI subsystem
    globals->get_AI_mgr()->update(1);	// FIXME - use real dt.

    // Run flight model

    // Calculate model iterations needed for next frame
    elapsed += remainder;

    global_multi_loop = (long)(((double)elapsed * 0.000001) * 
			      fgGetInt("/sim/model-hz"));
    remainder = elapsed - ( (global_multi_loop*1000000) / 
			    fgGetInt("/sim/model-hz") );
    SG_LOG( SG_ALL, SG_DEBUG, 
	    "Model iterations needed = " << global_multi_loop
	    << ", new remainder = " << remainder );
	
    // chop max interations to something reasonable if the sim was
    // delayed for an excesive amount of time
    if ( global_multi_loop > 2.0 * fgGetInt("/sim/model-hz") ) {
	global_multi_loop = (int)(2.0 * fgGetInt("/sim/model-hz") );
	remainder = 0;
    }

    // flight model
    if ( global_multi_loop > 0 ) {
	fgUpdateTimeDepCalcs();
    } else {
	SG_LOG( SG_ALL, SG_DEBUG, 
		"Elapsed time is zero ... we're zinging" );
    }

#if ! defined( macintosh )
    // Do any I/O channel work that might need to be done
    fgIOProcess();
#endif

    // see if we need to load any new scenery tiles
    double visibility_meters = fgGetDouble("/environment/visibility-m");
    global_tile_mgr.update( longitude->getDoubleValue(),
			    latitude->getDoubleValue(), visibility_meters );

    // see if we need to load any deferred-load textures
    material_lib.load_next_deferred();

    // Run audio scheduler
#ifdef ENABLE_AUDIO_SUPPORT
    if ( fgGetBool("/sim/sound/audible")
           && globals->get_soundmgr()->is_working() ) {
	globals->get_fx()->update(1); // FIXME: use dt
	globals->get_soundmgr()->update(1); // FIXME: use dt
    }
#endif

    // redraw display
    fgRenderFrame();

    SG_LOG( SG_ALL, SG_DEBUG, "" );
}


// This is the top level master main function that is registered as
// our idle funciton

// The first few passes take care of initialization things (a couple
// per pass) and once everything has been initialized fgMainLoop from
// then on.

static void fgIdleFunction ( void ) {
    // printf("idle state == %d\n", idle_state);

    if ( idle_state == 0 ) {
	// Initialize the splash screen right away
	if ( fgGetBool("/sim/startup/splash-screen") ) {
	    fgSplashInit();
	}
	
	idle_state++;
    } else if ( idle_state == 1 ) {
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

        // This is an 'easter egg' which is kind of a hard thing to
        // hide in an open source project -- so I won't try very hard
        // to hide it.  If you are looking at this now, please, you
        // don't want to ruin the surprise. :-) Just forget about it
        // and don't look any further; unless it is causing an actual
        // problem for you, which it shouldn't, and I hope it doesn't!
        // :-)
        if ( globals->get_time_params()->getGmt()->tm_mon == 4 
             && globals->get_time_params()->getGmt()->tm_mday == 19 )
        {
            string url = "http://www.flightgear.org/Music/invasion.mp3";
#if defined( __CYGWIN__ ) || defined( WIN32 )
            string command = "start /m " + url;
#else
            string command = "mpg123 " + url + "> /dev/null 2>&1";
#endif
            system( command.c_str() );
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
	// a subsystem to flightgear, its initialization call should
	// located in this routine.
	if( !fgInitSubsystems()) {
	    SG_LOG( SG_GENERAL, SG_ALERT,
		    "Subsystem initializations failed ..." );
	    exit(-1);
	}

	idle_state++;
    } else if ( idle_state == 4 ) {
	// setup OpenGL view parameters
	fgInitVisuals();

	idle_state++;
    } else if ( idle_state == 5 ) {

	idle_state++;
    } else if ( idle_state == 6 ) {
	// sleep(1);
	idle_state = 1000;

	cout << "Panel visible = " << fgPanelVisible() << endl;
	fgReshape( fgGetInt("/sim/startup/xsize"),
		   fgGetInt("/sim/startup/ysize") );
    } 

    if ( idle_state == 1000 ) {
	// We've finished all our initialization steps, from now on we
	// run the main loop.

	glutIdleFunc(fgMainLoop);
    } else {
	if ( fgGetBool("/sim/startup/splash-screen") ) {
	    fgSplashUpdate(0.0);
	}
    }
}

// options.cxx needs to see this for toggle_panel()
// Handle new window size or exposure
void fgReshape( int width, int height ) {
    int view_h;

    if ( (!fgGetBool("/sim/virtual-cockpit"))
	 && fgPanelVisible() && idle_state == 1000 ) {
	view_h = (int)(height * (current_panel->getViewHeight() -
				 current_panel->getYOffset()) / 768.0);
    } else {
	view_h = height;
    }

    // for all views
    FGViewMgr *viewmgr = globals->get_viewmgr();
    for ( int i = 0; i < viewmgr->size(); ++i ) {
      viewmgr->get_view(i)->
	set_aspect_ratio((float)view_h / (float)width);
    }

    glViewport( 0, (GLint)(height - view_h), (GLint)(width), (GLint)(view_h) );

    fgSetInt("/sim/startup/xsize", width);
    fgSetInt("/sim/startup/ysize", height);
    guiInitMouse(width, height);

    ssgSetFOV( viewmgr->get_current_view()->get_h_fov(),
	       viewmgr->get_current_view()->get_v_fov() );

    fgHUDReshape();
}

// Initialize GLUT and define a main window
int fgGlutInit( int *argc, char **argv ) {

#if !defined( macintosh )
    // GLUT will extract all glut specific options so later on we only
    // need wory about our own.
    glutInit(argc, argv);
#endif

    // Define Display Parameters
    glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

    SG_LOG( SG_GENERAL, SG_INFO, "Opening a window: " <<
	    fgGetInt("/sim/startup/xsize") << "x"
	    << fgGetInt("/sim/startup/ysize") );

    // Define initial window size
    glutInitWindowSize( fgGetInt("/sim/startup/xsize"),
			fgGetInt("/sim/startup/ysize") );

    // Initialize windows
    if ( !fgGetBool("/sim/startup/game-mode")) {
	// Open the regular window
	glutCreateWindow("FlightGear");
#ifndef GLUT_WRONG_VERSION
    } else {
	// Open the cool new 'game mode' window
	char game_mode_str[256];
//#define SYNC_OPENGL_WITH_DESKTOP_SETTINGS
#if defined(WIN32) && defined(SYNC_OPENGL_WITH_DESKTOP_SETTINGS)
#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#define ENUM_REGISTRY_SETTINGS      ((DWORD)-2)
#endif

	DEVMODE dm;
	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	fgSetInt("/sim/startup/xsize", dm.dmPelsWidth);
	fgSetInt("/sim/startup/ysize", dm.dmPelsHeight);
	glutInitWindowSize( fgGetInt("/sim/startup/xsize"),
						fgGetInt("/sim/startup/ysize") );
	sprintf( game_mode_str, "%dx%d:%d@%d",
			 dm.dmPelsWidth,
			 dm.dmPelsHeight,
			 dm.dmBitsPerPel,
			 dm.dmDisplayFrequency );
#else
	// Open the cool new 'game mode' window
	sprintf( game_mode_str, "width=%d height=%d bpp=%d",
		 fgGetInt("/sim/startup/xsize"),
		 fgGetInt("/sim/startup/ysize"),
		 fgGetInt("/sim/rendering/bits-per-pixel"));

#endif // HAVE_WINDOWS_H
	SG_LOG( SG_GENERAL, SG_INFO, 
		"game mode params = " << game_mode_str );
	glutGameModeString( game_mode_str );
	glutEnterGameMode();
#endif // GLUT_WRONG_VERSION
    }

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

    return 1;
}


// Initialize GLUT event handlers
int fgGlutInitEvents( void ) {
    // call fgReshape() on window resizes
    glutReshapeFunc( fgReshape );

    // keyboard and mouse callbacks are set in FGInput::init

#ifdef FG_OLD_MOUSE
    // call guiMouseFunc() whenever our little rodent is used
    glutMouseFunc ( guiMouseFunc );
    glutMotionFunc (guiMotionFunc );
    glutPassiveMotionFunc (guiMotionFunc );
#endif

    // call fgMainLoop() whenever there is
    // nothing else to do
    glutIdleFunc( fgIdleFunction );

    // draw the scene
    glutDisplayFunc( fgRenderFrame );

    return 1;
}


// Main loop
int mainLoop( int argc, char **argv ) {

#if defined( macintosh )
    freopen ("stdout.txt", "w", stdout );
    freopen ("stderr.txt", "w", stderr );
    argc = ccommand( &argv );
#endif

    // set default log levels
     sglog().setLogLevels( SG_ALL, SG_INFO );

    string version;
#ifdef FLIGHTGEAR_VERSION
    version = FLIGHTGEAR_VERSION;
#else
    version = "unknown version";
#endif
    SG_LOG( SG_GENERAL, SG_INFO, "FlightGear:  Version "
	    << version << endl );

    // Allocate global data structures.  This needs to happen before
    // we parse command line options

    globals = new FGGlobals;

#if defined(FG_NEW_ENVIRONMENT)
    globals->set_environment_mgr(new FGEnvironmentMgr);
#endif

    // seed the random number generater
    sg_srandom_time();

    SGRoute *route = new SGRoute;
    globals->set_route( route );

    FGControls *controls = new FGControls;
    globals->set_controls( controls );

    string_list *col = new string_list;
    globals->set_channel_options_list( col );

    // Scan the config file(s) and command line options to see if
    // fg_root was specified (ignore all other options for now)
    fgInitFGRoot(argc, argv);

    // Check for the correct base package version
    string base_version = fgBasePackageVersion();
    if ( !(base_version == "0.7.9") ) {
        // tell the operator how to use this application
        fgUsage();

	SG_LOG( SG_GENERAL, SG_ALERT, "Base package check failed ... "
		<< "Found version " << base_version << " at: "
                << globals->get_fg_root() );
        SG_LOG( SG_GENERAL, SG_ALERT, "Please upgrade to version 0.7.9" );
	exit(-1);
    }

    // Initialize the Aircraft directory to "" (UIUC)
    aircraft_dir = "";

    // Load the configuration parameters
    if ( !fgInitConfig(argc, argv) ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Config option parsing failed ..." );
	exit(-1);
    }

    // Initialize the Window/Graphics environment.
    if( !fgGlutInit(&argc, argv) ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "GLUT initialization failed ..." );
	exit(-1);
    }

    // Initialize the various GLUT Event Handlers.
    if( !fgGlutInitEvents() ) {
 	SG_LOG( SG_GENERAL, SG_ALERT, 
 		"GLUT event handler initialization failed ..." );
 	exit(-1);
    }

    // Initialize plib net interface
    netInit( &argc, argv );

    // Initialize ssg (from plib).  Needs to come before we do any
    // other ssg stuff, but after opengl/glut has been initialized.
    ssgInit();

    // Initialize the user interface (we need to do this before
    // passing off control to glut and before fgInitGeneral to get our
    // fonts !!!
    guiInit();

#ifdef GL_EXT_texture_lod_bias
    glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, -0.5 ) ;
#endif

    // Set position relative to glide slope if requested
    fgSetPosFromGlideSlope();

    // set current_options lon/lat if an airport id is specified
    // cout << "3. airport_id = " << fgGetString("/sim/startup/airport-id") << endl;
    if ( fgGetString("/sim/startup/airport-id")[0] != '\0' ) {
	// fgSetPosFromAirportID( fgGetString("/sim/startup/airport-id") );
	fgSetPosFromAirportIDandHdg( fgGetString("/sim/startup/airport-id"),
				     fgGetDouble("/orientation/heading-deg") );
        // set tower position (a little off the heading for single runway airports)
        fgSetTowerPosFromAirportID( fgGetString("/sim/startup/airport-id"), fgGetDouble("orientation/heading") );
    }

    SGTime *t = fgInitTime();
    globals->set_time_params( t );

    // Do some quick general initializations
    if( !fgInitGeneral()) {
	SG_LOG( SG_GENERAL, SG_ALERT, 
		"General initializations failed ..." );
	exit(-1);
    }

    SGPath modelpath( globals->get_fg_root() );
    ssgModelPath( (char *)modelpath.c_str() );
  
    // Scene graph root
    globals->set_scene_graph(new ssgRoot);
    globals->get_scene_graph()->setName( "Scene" );

    lighting = new ssgRoot;
    lighting->setName( "Lighting" );

    // Terrain branch
    globals->set_terrain_branch(new ssgBranch);
    globals->get_terrain_branch()->setName( "Terrain" );
    globals->get_scene_graph()->addKid( globals->get_terrain_branch() );

    globals->set_models_branch(new ssgBranch);
    globals->get_models_branch()->setName( "Models" );
    globals->get_scene_graph()->addKid( globals->get_models_branch() );

    globals->set_aircraft_branch(new ssgBranch);
    globals->get_aircraft_branch()->setName( "Aircraft" );
    globals->get_scene_graph()->addKid( globals->get_aircraft_branch() );

    // Lighting
    globals->set_gnd_lights_branch(new ssgBranch);
    globals->get_gnd_lights_branch()->setName( "Ground Lighting" );
    lighting->addKid( globals->get_gnd_lights_branch() );

    globals->set_rwy_lights_branch(new ssgBranch);
    globals->get_rwy_lights_branch()->setName( "Runway Lighting" );
    lighting->addKid( globals->get_rwy_lights_branch() );

    ////////////////////////////////////////////////////////////////////
    // Initialize the general model subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->set_model_mgr(new FGModelMgr);
    globals->get_model_mgr()->init();
    globals->get_model_mgr()->bind();

    ////////////////////////////////////////////////////////////////////
    // Initialize the 3D aircraft model subsystem.
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


    // Initialize the sky
    SGPath ephem_data_path( globals->get_fg_root() );
    ephem_data_path.append( "Astro" );
    SGEphemeris *ephem = new SGEphemeris( ephem_data_path.c_str() );
    ephem->update( globals->get_time_params()->getMjd(),
		   globals->get_time_params()->getLst(),
		   0.0 );
    globals->set_ephem( ephem );

    thesky = new SGSky;

    SGPath sky_tex_path( globals->get_fg_root() );
    sky_tex_path.append( "Textures" );
    sky_tex_path.append( "Sky" );
    thesky->texture_path( sky_tex_path.str() );

    thesky->build( 550.0, 550.0,
		   globals->get_ephem()->getNumPlanets(), 
		   globals->get_ephem()->getPlanets(), 60000.0,
		   globals->get_ephem()->getNumStars(),
		   globals->get_ephem()->getStars(), 60000.0 );

    if ( fgGetBool("/environment/clouds/status") ) {
	// thesky->add_cloud_layer( 2000.0, 200.0, 50.0, 40000.0,
        //                          SG_CLOUD_OVERCAST );
	thesky->add_cloud_layer( fgGetDouble("/environment/clouds/altitude-ft") *
				 SG_FEET_TO_METER,
				 200.0, 50.0, 40000.0,
				 SG_CLOUD_MOSTLY_CLOUDY );
	// thesky->add_cloud_layer( 3000.0, 200.0, 50.0, 40000.0,
	//                          SG_CLOUD_MOSTLY_SUNNY );
	thesky->add_cloud_layer( 6000.0, 20.0, 10.0, 40000.0,
				 SG_CLOUD_CIRRUS );
    }

    // Initialize MagVar model
    SGMagVar *magvar = new SGMagVar();
    globals->set_mag( magvar );

    // airport = new ssgBranch;
    // airport->setName( "Airport Lighting" );
    // lighting->addKid( airport );

    // ADA
    fgLoadDCS();
    // ADA

#ifdef FG_NETWORK_OLK
    // Do the network intialization
    if ( fgGetBool("/sim/networking/network-olk") ) {
	printf("Multipilot mode %s\n",
	       fg_net_init( globals->get_scene_graph() ) );
    }
#endif

    // build our custom render states
    fgBuildRenderStates();
    
    // pass control off to the master GLUT event handler
    glutMainLoop();

    // we never actually get here ... but to avoid compiler warnings,
    // etc.
    return 0;
}


// $$$ end - added VS Renganathan, 15 Oct 2K
//         - added Venky         , 12 Nov 2K

#if defined(__linux__) && defined(__i386__)

static void handleFPE (int);

static void
initFPE ()
{
    fpu_control_t fpe_flags = 0;
    _FPU_GETCW(fpe_flags);
//     fpe_flags &= ~_FPU_MASK_IM;	// invalid operation
//     fpe_flags &= ~_FPU_MASK_DM;	// denormalized operand
//     fpe_flags &= ~_FPU_MASK_ZM;	// zero-divide
//     fpe_flags &= ~_FPU_MASK_OM;	// overflow
//     fpe_flags &= ~_FPU_MASK_UM;	// underflow
//     fpe_flags &= ~_FPU_MASK_PM;	// precision (inexact result)
    _FPU_SETCW(fpe_flags);
    signal(SIGFPE, handleFPE);
}

static void
handleFPE (int num)
{
  initFPE();
  SG_LOG(SG_GENERAL, SG_ALERT, "Floating point interrupt (SIGFPE)");
}
#endif

// Main entry point; catch any exceptions that have made it this far.
int main ( int argc, char **argv ) {

    // Enable floating-point exceptions for Linux/x86
#if defined(__linux__) && defined(__i386__)
    initFPE();
#endif

    // Enable floating-point exceptions for Windows
#if defined( _MSC_VER ) && defined( DEBUG )
    // Christian, we should document what this does
    _control87( _EM_INEXACT, _MCW_EM );
#endif

#if defined( HAVE_BC5PLUS )
    _control87(MCW_EM, MCW_EM);  /* defined in float.h */
#endif

    // FIXME: add other, more specific
    // exceptions.
    try {
        mainLoop(argc, argv);
    } catch (sg_throwable &t) {
				// We must use cerr rather than
				// logging, since logging may be
				// disabled.
        cerr << "Fatal error: " << t.getFormattedMessage()
	     << "\n (received from " << t.getOrigin() << ')' << endl;
        exit(1);
    }

    return 0;
}


void fgLoadDCS(void) {

    ssgEntity *ship_obj = NULL;

    char obj_filename[25];

    for ( int k = 0; k < 32; k++ ) {
        ship_pos[k]=NULL;
    }

    SGPath tile_path( globals->get_fg_root());
    tile_path.append( "Scenery" );
    tile_path.append( "Objects.txt" );
    sg_gzifstream in( tile_path.str() );
    if ( ! in.is_open() ) {
	SG_LOG( SG_TERRAIN, SG_ALERT, "Cannot open file: " << tile_path.str() );
    } else {

        SGPath modelpath( globals->get_fg_root() );
        modelpath.append( "Models" );
        modelpath.append( "Geometry" );
  
        SGPath texturepath( globals->get_fg_root() );
        texturepath.append( "Models" );
        texturepath.append( "Textures" );
 
        ssgModelPath( (char *)modelpath.c_str() );
        ssgTexturePath( (char *)texturepath.c_str() );

        ship_sel = new ssgSelector;

        char c;
        while ( ! in.eof() ) {
            in >> ::skipws;
            if ( in.get( c ) && c == '#' ) { 
                in >> skipeol;
            } else { 
                in.putback(c);
                in >> obj_filename >> obj_lat[objc] >> obj_lon[objc] >> obj_alt[objc];
                /* cout << endl << obj_filename << " " << obj_lat[objc] << " " << obj_lon[objc] <<  " " << obj_alt[objc] << endl;
                   int chj=getchar();*/
                
                obj_lon[objc] *=SGD_DEGREES_TO_RADIANS;
                obj_lat[objc] *=SGD_DEGREES_TO_RADIANS;
                
                ship_pos[objc] = new ssgTransform;
       
                // type "repeat" in objects.txt to load one more
                // instance of the last object.

                if ( strcmp(obj_filename,"repeat") != 0) {
                    ship_obj = ssgLoad( obj_filename );
                }
      
                if ( ship_obj != NULL ) {
					ship_obj->setName(obj_filename);
			        if (objc == 0)
						ship_obj->clrTraversalMaskBits( SSGTRAV_HOT );
					else
						ship_obj->setTraversalMaskBits( SSGTRAV_HOT );
                    ship_pos[objc]->addKid( ship_obj ); // add object to transform node
                    ship_sel->addKid( ship_pos[objc] ); // add transform node to selector
                    SG_LOG( SG_TERRAIN, SG_ALERT, "Loaded file: "
                            << obj_filename );
                } else {
                    SG_LOG( SG_TERRAIN, SG_ALERT, "Cannot open file: "
                            << obj_filename );
                }
            
				// temporary hack for deck lights - ultimately should move to PLib (when??)
				//const char *extn = file_extension ( obj_filename ) ;
				if ( objc == 1 ){
				    ssgVertexArray *lights = new ssgVertexArray( 100 );
					ssgVertexArray *lightpoints = new ssgVertexArray( 100 );
					ssgVertexArray *lightnormals = new ssgVertexArray( 100 );
					ssgVertexArray *lightdir = new ssgVertexArray( 100 );
					int ltype[500], light_type;
					static int ltcount = 0;
				    string token;
					sgVec3 rway_dir,rway_normal,lightpt;
					Point3D node;
					modelpath.append(obj_filename);
					sg_gzifstream in1( modelpath.str() );
					if ( ! in1.is_open() ) {
						SG_LOG( SG_TERRAIN, SG_ALERT, "Cannot open file: " << modelpath.str() );
					} else {
						while ( ! in1.eof() ) {
                                                        in1 >> ::skipws;
							if ( in1.get( c ) && c == '#' ) { 
								in1 >> skipeol;
							} else { 
								in1.putback(c);
								in1 >> token;
								//cout << token << endl;
								if ( token == "runway" ) {
									in1 >> node;
									sgSetVec3 (rway_dir, node[0], node[1], node[2] );			 
								} else if ( token == "edgelight" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );
									light_type = 1;
								} else if ( token == "taxi" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );			 
									light_type = 2;
								} else if ( token == "vasi" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );			 
									light_type = 3;
								} else if ( token == "threshold" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );			 
									light_type = 4;
								} else if ( token == "rabbit" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );
									light_type = 5;
								} else if ( token == "ols" ) {
									in1 >> node;
									sgSetVec3 (rway_ols, node[0], node[1], node[2] );
									light_type = 6;
								} else if ( token == "red" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );
									light_type = 7;
								} else if ( token == "green" ) {
									in1 >> node;
									sgSetVec3 (rway_normal, node[0], node[1], node[2] );
									light_type = 8;
								} else if ( token == "lp" ) {
									in1 >> node;
									sgSetVec3 (lightpt, node[0], node[1], node[2] );
									lightpoints->add( lightpt );
									lightnormals->add( rway_normal );
									lightdir->add( rway_dir );
									ltype[ltcount]= light_type;
									ltcount++;
								}
								if (in1.eof()) break;
							} 
						}  //while
	
						if ( lightpoints->getNum() ) {
							ssgBranch *lightpoints_branch;
							long int dummy = -999;
							dummy_tile = new FGTileEntry((SGBucket)dummy);
							dummy_tile->lightmaps_sequence = new ssgSelector;
							dummy_tile->ols_transform = new ssgTransform;

							// call function to generate the runway lights
							lightpoints_branch = 
							dummy_tile->gen_runway_lights( lightpoints, lightnormals,
																lightdir, ltype);
							lightpoints_brightness->addKid(lightpoints_branch);
							lightpoints_transform->addKid(lightpoints_brightness);
						    //dummy_tile->lightmaps_sequence->setTraversalMaskBits( SSGTRAV_HOT );
							lightpoints_transform->addKid( dummy_tile->lightmaps_sequence );
							lightpoints_transform->ref();
							globals->get_gnd_lights_branch()->addKid( lightpoints_transform );
						} 
					} //if in1 
                } //if objc
				// end hack for deck lights

                objc++;

                if (in.eof()) break;
            }
        } // while

        SG_LOG ( SG_TERRAIN, SG_ALERT, "Finished object processing." );

        globals->get_terrain_branch()->addKid( ship_sel ); //add selector node to root node 
    }

    return;
 }


void fgUpdateDCS (void) {

    // double eye_lat,eye_lon,eye_alt;
    // static double obj_head;
    double sl_radius,obj_latgc;
    // float nresultmat[4][4];
    // sgMat4 Trans,rothead,rotlon,rot180,rotlat,resultmat1,resultmat2,resultmat3;
    double bz[3];

    // Instantaneous Geodetic Lat/Lon/Alt of moving object
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    
    // Deck should be the first object in objects.txt in case of fdm=ada

    if (!strcmp(fgGetString("/sim/flight-model"), "ada")) {
		if ((fdm->get_iaux(1))==1)
		{
			obj_lat[1] = fdm->get_daux(1)*SGD_DEGREES_TO_RADIANS;
			obj_lon[1] = fdm->get_daux(2)*SGD_DEGREES_TO_RADIANS;
			obj_alt[1] = fdm->get_daux(3);
			obj_pitch[1] = fdm->get_faux(1);
			obj_roll[1] = fdm->get_faux(2);
		}
    }
    
    for ( int m = 0; m < objc; m++ ) {
    	//cout << endl <<  obj_lat[m]*SGD_RADIANS_TO_DEGREES << " " << obj_lon[m]*SGD_RADIANS_TO_DEGREES << " " << obj_alt[m] << " " << objc << endl;
	//int v=getchar();

        //Geodetic to Geocentric angles for rotation
	sgGeodToGeoc(obj_lat[m],obj_alt[m],&sl_radius,&obj_latgc);

        //moving object gbs-posn in cartesian coords
        Point3D obj_posn = Point3D( obj_lon[m],obj_lat[m],obj_alt[m]);
        Point3D obj_pos = sgGeodToCart( obj_posn );

        // Translate moving object w.r.t eye
        Point3D Objtrans = obj_pos-scenery.get_center();
        bz[0]=Objtrans.x();
        bz[1]=Objtrans.y();
        bz[2]=Objtrans.z();

       // rotate dynamic objects for lat,lon & alt and other motion about its axes
	
	sgMat4 sgTRANS;
	sgMakeTransMat4( sgTRANS, bz[0],bz[1],bz[2]);

	sgVec3 ship_fwd,ship_rt,ship_up;
	sgSetVec3( ship_fwd, 1.0, 0.0, 0.0);//east,roll
	sgSetVec3( ship_rt, 0.0, 1.0, 0.0);//up,pitch
	sgSetVec3( ship_up, 0.0, 0.0, 1.0); //north,yaw

	sgMat4 sgROT_lon, sgROT_lat, sgROT_hdg, sgROT_pitch, sgROT_roll;
	sgMakeRotMat4( sgROT_lon, obj_lon[m]*SGD_RADIANS_TO_DEGREES, ship_up );
	sgMakeRotMat4( sgROT_lat, 90-obj_latgc*SGD_RADIANS_TO_DEGREES, ship_rt );
	sgMakeRotMat4( sgROT_hdg, 180.0, ship_up );
	sgMakeRotMat4( sgROT_pitch, obj_pitch[m], ship_rt );
	sgMakeRotMat4( sgROT_roll, obj_roll[m], ship_fwd );
	
	sgMat4 sgTUX;
	sgCopyMat4( sgTUX, sgROT_hdg );
	sgPostMultMat4( sgTUX, sgROT_pitch );
	sgPostMultMat4( sgTUX, sgROT_roll );
	sgPostMultMat4( sgTUX, sgROT_lat );
	sgPostMultMat4( sgTUX, sgROT_lon );
	sgPostMultMat4( sgTUX, sgTRANS );

	sgCoord shippos;
	sgSetCoord(&shippos, sgTUX );
	ship_pos[m]->setTransform( &shippos );
	// temporary hack for deck lights - ultimately should move to PLib (when ??)
	if (m == 1) {
		if (lightpoints_transform) {
			lightpoints_transform->setTransform( &shippos );
			float sun_angle = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
			if ( sun_angle > 89 ) {
				lightpoints_brightness->select(0x01);
			} else {
				lightpoints_brightness->select(0x00);
			}
		}

		float elev;
		sgVec3 rp,to;
		float *vp;
		float alt;
		float ref_elev;
		sgXformPnt3( rp, rway_ols, sgTUX );
		vp = globals->get_current_view()->get_view_pos();
	    to[0] = rp[0]-vp[0];
	    to[1] = rp[1]-vp[1];
	    to[2] = rp[2]-vp[2];
		float dist = sgLengthVec3( to );
		alt = (current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER)-rway_ols[2];

		elev = asin(alt/dist)*SGD_RADIANS_TO_DEGREES;

	    ref_elev = elev - 3.75; // +ve above, -ve below
        
		unsigned int sel;
		sel = 0xFF;
// DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS
//		if (ref_elev > 0.51) sel = 0x21;
//		if ((ref_elev <= 0.51) & (ref_elev > 0.17)) sel = 0x22;
//		if ((ref_elev <= 0.17) & (ref_elev >= -0.17)) sel = 0x24;
//		if ((ref_elev < -0.17) & (ref_elev >= -0.51)) sel = 0x28;
//		if (ref_elev < -0.51) sel = 0x30;
// DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS
		dummy_tile->lightmaps_sequence->select(sel);

		sgVec3 up;
		sgCopyVec3 (up, ship_up);
		if (dist > 750) 
			sgScaleVec3 (up, 4.0*ref_elev*dist/750.0);
		else
			sgScaleVec3 (up, 4.0*ref_elev);
		dummy_tile->ols_transform->setTransform(up);
		//cout << "ref_elev  " << ref_elev << endl;
	}
    // end hack for deck lights

    }
    if ( ship_sel != NULL ) {
	ship_sel->select(0xFFFFFFFE); // first object is ownship, added to acmodel
    }
}

// $$$ end - added VS Renganathan, 15 Oct 2K
//           added Venky         , 12 Nov 2K
