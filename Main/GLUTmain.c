/**************************************************************************
 * GLUTmain.c -- top level sim routines
 *
 * Written by Curtis Olson for OpenGL, started May 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef WIN32
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>
#include <stdio.h>

#include <Main/GLUTkey.h>
#include <Main/fg_init.h>
#include <Main/fg_debug.h>
#include <Main/views.h>

#include <Include/fg_constants.h>
#include <Include/general.h>

#include <Aircraft/aircraft.h>
#include <Astro/moon.h>
#include <Astro/planets.h>
#include <Astro/sky.h>
#include <Astro/stars.h>
#include <Astro/sun.h>
#include <Cockpit/cockpit.h>
#include <Joystick/joystick.h>
#include <Math/fg_geodesy.h>
#include <Math/mat3.h>
#include <Math/polar.h>
#include <Scenery/scenery.h>
#include <Scenery/tilemgr.h>
#include <Time/event.h>
#include <Time/fg_time.h>
#include <Time/fg_timer.h>
#include <Time/sunpos.h>
#include <Weather/weather.h>


/* This is a record containing global housekeeping information */
struct fgGENERAL general;

/* view parameters */
static GLfloat win_ratio = 1.0;
static GLint winWidth, winHeight;

/* temporary hack */
/* pointer to scenery structure */
/* static GLint scenery, runway; */

/* double Simtime; */

/* Another hack */
int use_signals = 0;

/* Yet another hack. This one used by the HUD code. Michele */
int show_hud;

/* Yet another other hack. Used for my prototype instrument code. (Durk) */
int displayInstruments; 


/**************************************************************************
 * fgInitVisuals() -- Initialize various GL/view parameters
 **************************************************************************/

static void fgInitVisuals( void ) {
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgWEATHER *w;

    l = &cur_light_params;
    t = &cur_time_params;
    w = &current_weather;

    
    /* xglDisable( GL_DITHER ); */

    /* If enabled, normal vectors specified with glNormal are scaled
       to unit length after transformation.  See glNormal. */
    xglEnable( GL_NORMALIZE );

    xglEnable( GL_LIGHTING );
    xglEnable( GL_LIGHT0 );
    xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );

    xglFogi (GL_FOG_MODE, GL_LINEAR);
    /* xglFogf (GL_FOG_START, 1.0); */
    xglFogf (GL_FOG_END, w->visibility);
    /* xglFogf (GL_FOG_DENSITY, w->visibility); */
    xglHint (GL_FOG_HINT, GL_NICEST /* GL_FASTEST */ );

    /* draw wire frame */
    /* xglPolygonMode(GL_FRONT_AND_BACK,GL_LINE); */
}


/**************************************************************************
 * Update the view volume, position, and orientation
 **************************************************************************/

static void fgUpdateViewParams( void ) {
    fgFLIGHT *f;
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgVIEW *v;

    f = current_aircraft.flight;
    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    fgViewUpdate(f, v, l);

    if (displayInstruments)
      {
	xglViewport(0, (GLint)(winHeight / 2 ) , (GLint)winWidth, (GLint)winHeight / 2);
	/* Tell GL we are about to modify the projection parameters */    
	xglMatrixMode(GL_PROJECTION);
	xglLoadIdentity();
	gluPerspective(55.0, 2.0/win_ratio, 1.0, 100000.0);
      }
    else
      {
	xglViewport(0, 0 , (GLint)winWidth, (GLint) winHeight);
	/* Tell GL we are about to modify the projection parameters */    
	xglMatrixMode(GL_PROJECTION);
	xglLoadIdentity();
	gluPerspective(55.0, 1.0/win_ratio, 1.0, 100000.0);
      }

    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    
    /* set up our view volume (default) */
    gluLookAt(v->view_pos.x, v->view_pos.y, v->view_pos.z,
	       v->view_pos.x + v->view_forward[0], 
	       v->view_pos.y + v->view_forward[1], 
	       v->view_pos.z + v->view_forward[2],
	       v->view_up[0], v->view_up[1], v->view_up[2]);

    /* lock view horizontally towards sun (testing) */
    /* gluLookAt(v->view_pos.x, v->view_pos.y, v->view_pos.z,
	       v->view_pos.x + v->surface_to_sun[0], 
	       v->view_pos.y + v->surface_to_sun[1], 
	       v->view_pos.z + v->surface_to_sun[2],
	       v->view_up[0], v->view_up[1], v->view_up[2]); */

    /* lock view horizontally towards south (testing) */
    /* gluLookAt(v->view_pos.x, v->view_pos.y, v->view_pos.z,
	       v->view_pos.x + v->surface_south[0], 
	       v->view_pos.y + v->surface_south[1], 
	       v->view_pos.z + v->surface_south[2],
	       v->view_up[0], v->view_up[1], v->view_up[2]); */

    /* set the sun position */
    xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );
}


/*************************************************************************
 * Draw a basic instrument panel
 ************************************************************************/
static void fgUpdateInstrViewParams( void ) {
    xglViewport(0, 0 , (GLint)winWidth, (GLint)winHeight / 2);
  
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


/**************************************************************************
 * Update all Visuals (redraws anything graphics related)
 **************************************************************************/

static void fgRenderFrame( void ) {
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgVIEW *v;
    double angle;
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    /* update view volume parameters */
    fgUpdateViewParams();

    xglClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    /* Tell GL we are switching to model view parameters */
    xglMatrixMode(GL_MODELVIEW);
    /* xglLoadIdentity(); */

    /* draw sky */
    xglDisable( GL_DEPTH_TEST );
    xglDisable( GL_LIGHTING );
    xglDisable( GL_CULL_FACE );
    xglDisable( GL_FOG );
    xglShadeModel( GL_SMOOTH );
    fgSkyRender();

    /* setup transformation for drawing astronomical objects */
    xglPushMatrix();
    /* Translate to view position */
    xglTranslatef( v->view_pos.x, v->view_pos.y, v->view_pos.z );
    /* Rotate based on gst (sidereal time) */
    angle = t->gst * 15.041085; /* should be 15.041085, Curt thought it was 15*/
    /* printf("Rotating astro objects by %.2f degrees\n",angle); */
    xglRotatef( angle, 0.0, 0.0, -1.0 );

    /* draw stars and planets */
    fgStarsRender();
    fgPlanetsRender();

    /* draw the sun */
    fgSunRender();

    /* render the moon */
    xglEnable( GL_LIGHTING );
    /* set lighting parameters */
    xglLightfv(GL_LIGHT0, GL_AMBIENT, white );
    xglLightfv(GL_LIGHT0, GL_DIFFUSE, white );
    xglEnable( GL_CULL_FACE );
    
    /* Let's try some blending technique's (Durk)*/
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    fgMoonRender();
    glDisable(GL_BLEND);

    xglPopMatrix();

    /* draw scenery */
    xglShadeModel( /* GL_FLAT */ GL_SMOOTH ); 
    xglEnable( GL_DEPTH_TEST );
    xglEnable( GL_FOG );
    xglFogfv (GL_FOG_COLOR, l->fog_color);
    /* set lighting parameters */
    xglLightfv(GL_LIGHT0, GL_AMBIENT, l->scene_ambient );
    xglLightfv(GL_LIGHT0, GL_DIFFUSE, l->scene_diffuse );
    fgTileMgrRender();
    /* fgSceneryRender(); */

    /* display HUD */
    if( show_hud ) {
     	fgCockpitUpdate();
    }

    /* display instruments */
    if (displayInstruments) {
	fgUpdateInstrViewParams();
    }

    #ifdef GLUT
      xglutSwapBuffers();
    #endif
}


/**************************************************************************
 * Update internal time dependent calculations (i.e. flight model)
 **************************************************************************/

void fgUpdateTimeDepCalcs(int multi_loop) {
    fgFLIGHT *f;
    struct fgTIME *t;
    struct fgVIEW *v;
    int i;

    f = current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;

    /* update the flight model */
    if ( multi_loop < 0 ) {
	multi_loop = DEFAULT_MULTILOOP;
    }

    /* printf("updating flight model x %d\n", multi_loop); */
    fgFlightModelUpdate(FG_LARCSIM, f, multi_loop);

    /* update the view angle */
    for ( i = 0; i < multi_loop; i++ ) {
	if ( fabs(v->goal_view_offset - v->view_offset) < 0.05 ) {
	    v->view_offset = v->goal_view_offset;
	    break;
	} else {
	    /* move v->view_offset towards v->goal_view_offset */
	    if ( v->goal_view_offset > v->view_offset ) {
		if ( v->goal_view_offset - v->view_offset < FG_PI ) {
		    v->view_offset += 0.01;
		} else {
		    v->view_offset -= 0.01;
		}
	    } else {
		if ( v->view_offset - v->goal_view_offset < FG_PI ) {
		    v->view_offset -= 0.01;
		} else {
		    v->view_offset += 0.01;
		}
	    }
	    if ( v->view_offset > FG_2PI ) {
		v->view_offset -= FG_2PI;
	    } else if ( v->view_offset < 0 ) {
		v->view_offset += FG_2PI;
	    }
	}
    }
}


void fgInitTimeDepCalcs( void ) {
    /* initialize timer */

#ifdef USE_ITIMER
    fgTimerInit( 1.0 / DEFAULT_TIMER_HZ, fgUpdateTimeDepCalcs );
#endif USE_ITIMER

}


/**************************************************************************
 * Scenery management routines
 **************************************************************************/

/* static void fgSceneryInit_OLD() { */
    /* make scenery */
/*     scenery = fgSceneryCompile_OLD();
    runway = fgRunwayHack_OLD(0.69, 53.07);
} */


/* create the scenery */
/* GLint fgSceneryCompile_OLD() {
    GLint scenery;

    scenery = mesh2GL(mesh_ptr_OLD);

    return(scenery);
}
*/

/* hack in a runway */
/* GLint fgRunwayHack_OLD(double width, double length) {
    static GLfloat concrete[4] = { 0.5, 0.5, 0.5, 1.0 };
    static GLfloat line[4]     = { 0.9, 0.9, 0.9, 1.0 };
    int i;
    int num_lines = 16;
    float line_len, line_width_2, cur_pos;

    runway = xglGenLists(1);
    xglNewList(runway, GL_COMPILE);
    */
    /* draw concrete */
/*    xglBegin(GL_POLYGON);
    xglMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, concrete );
    xglNormal3f(0.0, 0.0, 1.0);

    xglVertex3d( 0.0,   -width/2.0, 0.0);
    xglVertex3d( 0.0,    width/2.0, 0.0);
    xglVertex3d(length,  width/2.0, 0.0);
    xglVertex3d(length, -width/2.0, 0.0);
    xglEnd();
    */
    /* draw center line */
/*    xglMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, line );
    line_len = length / ( 2 * num_lines + 1);
    printf("line_len = %.3f\n", line_len);
    line_width_2 = 0.02;
    cur_pos = line_len;
    for ( i = 0; i < num_lines; i++ ) {
	xglBegin(GL_POLYGON);
	xglVertex3d( cur_pos, -line_width_2, 0.005);
	xglVertex3d( cur_pos,  line_width_2, 0.005);
	cur_pos += line_len;
	xglVertex3d( cur_pos,  line_width_2, 0.005);
	xglVertex3d( cur_pos, -line_width_2, 0.005);
	cur_pos += line_len;
	xglEnd();
    }

    xglEndList();

    return(runway);
}
*/

/* draw the scenery */
/*static void fgSceneryDraw_OLD() {
    static float z = 32.35;

    xglPushMatrix();

    xglCallList(scenery);

    printf("*** Drawing runway at %.2f\n", z);

    xglTranslatef( -398391.28, 120070.41, 32.35);
    xglRotatef(170.0, 0.0, 0.0, 1.0);
    xglCallList(runway);

    xglPopMatrix();
}
*/


/* What should we do when we have nothing else to do?  How about get
 * ready for the next move and update the display? */
static void fgMainLoop( void ) {
    static int remainder = 0;
    int elapsed, multi_loop;
    double cur_elev;
    /* double joy_x, joy_y; */
    /* int joy_b1, joy_b2; */
    fgAIRCRAFT *a;
    fgFLIGHT *f;
    struct fgTIME *t;

    fgPrintf( FG_ALL, FG_DEBUG, "Running Main Loop\n");
    fgPrintf( FG_ALL, FG_DEBUG, "======= ==== ====\n");

    a = &current_aircraft;
    f = a->flight;
    t = &cur_time_params;

    /* update "time" */
    fgTimeUpdate(f, t);

    /* Read joystick */
    /* fgJoystickRead( &joy_x, &joy_y, &joy_b1, &joy_b2 );
    printf( "Joystick X %f  Y %f  B1 %d  B2 %d\n",  
	    joy_x, joy_y, joy_b1, joy_b2 );
    fgElevSet( -joy_y );
    fgAileronSet( joy_x ); */

    /* Calculate model iterations needed */
    elapsed = fgGetTimeInterval();
    fgPrintf( FG_ALL, FG_BULK, 
	      "Time interval is = %d, previous remainder is = %d\n", 
	      elapsed, remainder);
    fgPrintf( FG_ALL, FG_BULK, 
	      "--> Frame rate is = %.2f\n", 1000.0 / (float)elapsed);
    elapsed += remainder;

    multi_loop = (int)(((float)elapsed * 0.001) * DEFAULT_MODEL_HZ);
    remainder = elapsed - ((multi_loop*1000) / DEFAULT_MODEL_HZ);
    fgPrintf( FG_ALL, FG_BULK, 
	      "Model iterations needed = %d, new remainder = %d\n", 
	      multi_loop, remainder);

    /* Run flight model */
    if ( ! use_signals ) {
	/* flight model */
	fgUpdateTimeDepCalcs(multi_loop);
    }

    /* I'm just sticking this here for now, it should probably move 
     * eventually */
    /* cur_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC, 
			       FG_Latitude  * RAD_TO_ARCSEC); */
    /* there is no ground collision detection really, so for now I
     * just hard code the ground elevation to be 0 */
    cur_elev = 0;

    /* printf("Ground elevation is %.2f meters here.\n", cur_elev); */
    /* FG_Runway_altitude = cur_elev * METER_TO_FEET; */

    if ( FG_Altitude * FEET_TO_METER < cur_elev + 3.758099) {
	/* set this here, otherwise if we set runway height above our
	   current height we get a really nasty bounce. */
	FG_Runway_altitude = FG_Altitude - 3.758099;

	/* now set aircraft altitude above ground */
	FG_Altitude = cur_elev * METER_TO_FEET + 3.758099;
	fgPrintf( FG_ALL, FG_BULK, "<*> resetting altitude to %.0f meters\n", 
	       FG_Altitude * FEET_TO_METER);
    }

    fgAircraftOutputCurrent(a);

    /* see if we need to load any new scenery tiles */
    fgTileMgrUpdate();

    /* Process/manage pending events */
    fgEventProcess();

    /* redraw display */
    fgRenderFrame();

    fgPrintf( FG_ALL, FG_DEBUG, "\n");
}


/**************************************************************************
 * Handle new window size or exposure
 **************************************************************************/

static void fgReshape( int width, int height ) {
    /* Do this so we can call fgReshape(0,0) ourselves without having to know
     * what the values of width & height are. */
    if ( (height > 0) && (width > 0) ) {
	win_ratio = (GLfloat) height / (GLfloat) width;
    }

    winWidth = width;
    winHeight = height;

    /* Inform gl of our view window size (now handled elsewhere) */
    /* xglViewport(0, 0, (GLint)width, (GLint)height); */

    fgUpdateViewParams();
    
    /* xglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); */
}


/**************************************************************************
 * Main ...
 **************************************************************************/

int main( int argc, char *argv[] ) {
    fgFLIGHT *f;

    f = current_aircraft.flight;

    printf("Flight Gear:  Version %s\n\n", VERSION);

    /**********************************************************************
     * Initialize the Window/Graphics environment.
     **********************************************************************/

    #ifdef GLUT
      /* initialize GLUT */
      xglutInit(&argc, argv);

      /* Define Display Parameters */
      xglutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

      /* Define initial window size */
      xglutInitWindowSize(640, 480);

      /* Initialize windows */
      xglutCreateWindow("Flight Gear");
    #endif

    /* This is the general house keeping init routine */
    fgInitGeneral();

    /* This is the top level init routine which calls all the other
     * subsystem initialization routines.  If you are adding a
     * subsystem to flight gear, its initialization call should
     * located in this routine.*/
    fgInitSubsystems();

    /* setup view parameters, only makes GL calls */
    fgInitVisuals();

    if ( use_signals ) {
	/* init timer routines, signals, etc.  Arrange for an alarm
	   signal to be generated, etc. */
	fgInitTimeDepCalcs();
    }

   /**********************************************************************
     * Initialize the Event Handlers.
     **********************************************************************/

    #ifdef GLUT
      /* call fgReshape() on window resizes */
      xglutReshapeFunc( fgReshape );

      /* call key() on keyboard event */
      xglutKeyboardFunc( GLUTkey );
      glutSpecialFunc( GLUTspecialkey );

      /* call fgMainLoop() whenever there is nothing else to do */
      xglutIdleFunc( fgMainLoop );

      /* draw the scene */
      xglutDisplayFunc( fgRenderFrame );

      /* pass control off to the GLUT event handler */
      glutMainLoop();
    #endif

    return(0);
}


#ifdef __SUNPRO_CC
    extern "C" {
	void __eprintf( void ) {
	}
    }
#endif

/* $Log$
/* Revision 1.60  1998/02/11 02:50:40  curt
/* Minor changes.
/*
 * Revision 1.59  1998/02/09 22:56:54  curt
 * Removed "depend" files from cvs control.  Other minor make tweaks.
 *
 * Revision 1.58  1998/02/09 15:07:49  curt
 * Minor tweaks.
 *
 * Revision 1.57  1998/02/07 15:29:40  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.56  1998/02/03 23:20:23  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.55  1998/02/02 20:53:58  curt
 * Incorporated Durk's changes.
 *
 * Revision 1.54  1998/01/31 00:43:10  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.53  1998/01/27 18:35:54  curt
 * Minor tweaks.
 *
 * Revision 1.52  1998/01/27 00:47:56  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.51  1998/01/26 15:57:05  curt
 * Tweaks for dynamic scenery development.
 *
 * Revision 1.50  1998/01/19 19:27:07  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.49  1998/01/19 18:40:31  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.48  1998/01/19 18:35:46  curt
 * Minor tweaks and fixes for cygwin32.
 *
 * Revision 1.47  1998/01/13 00:23:08  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.46  1998/01/08 02:22:06  curt
 * Beginning to integrate Tile management subsystem.
 *
 * Revision 1.45  1998/01/07 03:18:55  curt
 * Moved astronomical stuff from .../Src/Scenery to .../Src/Astro/
 *
 * Revision 1.44  1997/12/30 22:22:31  curt
 * Further integration of event manager.
 *
 * Revision 1.43  1997/12/30 20:47:43  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.42  1997/12/30 16:36:47  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.41  1997/12/30 13:06:56  curt
 * A couple lighting tweaks ...
 *
 * Revision 1.40  1997/12/30 01:38:37  curt
 * Switched back to per vertex normals and smooth shading for terrain.
 *
 * Revision 1.39  1997/12/22 23:45:45  curt
 * First stab at sunset/sunrise sky glow effects.
 *
 * Revision 1.38  1997/12/22 04:14:28  curt
 * Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
 *
 * Revision 1.37  1997/12/19 23:34:03  curt
 * Lot's of tweaking with sky rendering and lighting.
 *
 * Revision 1.36  1997/12/19 16:44:57  curt
 * Working on scene rendering order and options.
 *
 * Revision 1.35  1997/12/18 23:32:32  curt
 * First stab at sky dome actually starting to look reasonable. :-)
 *
 * Revision 1.34  1997/12/17 23:13:34  curt
 * Began working on rendering a sky.
 *
 * Revision 1.33  1997/12/15 23:54:45  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.32  1997/12/15 20:59:08  curt
 * Misc. tweaks.
 *
 * Revision 1.31  1997/12/12 21:41:25  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.30  1997/12/12 19:52:47  curt
 * Working on lightling and material properties.
 *
 * Revision 1.29  1997/12/11 04:43:54  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.28  1997/12/10 22:37:45  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.27  1997/12/09 05:11:54  curt
 * Working on tweaking lighting.
 *
 * Revision 1.26  1997/12/09 04:25:29  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.25  1997/12/08 22:54:09  curt
 * Enabled GL_CULL_FACE.
 *
 * Revision 1.24  1997/11/25 19:25:32  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.23  1997/11/15 18:16:34  curt
 * minor tweaks.
 *
 * Revision 1.22  1997/10/30 12:38:41  curt
 * Working on new scenery subsystem.
 *
 * Revision 1.21  1997/09/23 00:29:38  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.20  1997/09/22 14:44:19  curt
 * Continuing to try to align stars correctly.
 *
 * Revision 1.19  1997/09/18 16:20:08  curt
 * At dusk/dawn add/remove stars in stages.
 *
 * Revision 1.18  1997/09/16 22:14:51  curt
 * Tweaked time of day lighting equations.  Don't draw stars during the day.
 *
 * Revision 1.17  1997/09/16 15:50:29  curt
 * Working on star alignment and time issues.
 *
 * Revision 1.16  1997/09/13 02:00:06  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.15  1997/09/05 14:17:27  curt
 * More tweaking with stars.
 *
 * Revision 1.14  1997/09/05 01:35:53  curt
 * Working on getting stars right.
 *
 * Revision 1.13  1997/09/04 02:17:34  curt
 * Shufflin' stuff.
 *
 * Revision 1.12  1997/08/27 21:32:24  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.11  1997/08/27 03:30:16  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.10  1997/08/25 20:27:22  curt
 * Merged in initial HUD and Joystick code.
 *
 * Revision 1.9  1997/08/22 21:34:39  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.8  1997/08/19 23:55:03  curt
 * Worked on better simulating real lighting.
 *
 * Revision 1.7  1997/08/16 12:22:38  curt
 * Working on improving the lighting/shading.
 *
 * Revision 1.6  1997/08/13 20:24:56  curt
 * Changes due to changing sunpos interface.
 *
 * Revision 1.5  1997/08/06 21:08:32  curt
 * Sun position now *really* works (I think) ... I still have sun time warping
 * code in place, probably should remove it soon.
 *
 * Revision 1.4  1997/08/06 15:41:26  curt
 * Working on correct sun position.
 *
 * Revision 1.3  1997/08/06 00:24:22  curt
 * Working on correct real time sun lighting.
 *
 * Revision 1.2  1997/08/04 20:25:15  curt
 * Organizational tweaking.
 *
 * Revision 1.1  1997/08/02 18:45:00  curt
 * Renamed GLmain.c GLUTmain.c
 *
 * Revision 1.43  1997/08/02 16:23:47  curt
 * Misc. tweaks.
 *
 * Revision 1.42  1997/08/01 19:43:33  curt
 * Making progress with coordinate system overhaul.
 *
 * Revision 1.41  1997/07/31 22:52:37  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.40  1997/07/30 16:12:42  curt
 * Moved fg_random routines from Util/ to Math/
 *
 * Revision 1.39  1997/07/21 14:45:01  curt
 * Minor tweaks.
 *
 * Revision 1.38  1997/07/19 23:04:47  curt
 * Added an initial weather section.
 *
 * Revision 1.37  1997/07/19 22:34:02  curt
 * Moved PI definitions to ../constants.h
 * Moved random() stuff to ../Utils/ and renamed fg_random()
 *
 * Revision 1.36  1997/07/18 23:41:25  curt
 * Tweaks for building with Cygnus Win32 compiler.
 *
 * Revision 1.35  1997/07/18 14:28:34  curt
 * Hacked in some support for wind/turbulence.
 *
 * Revision 1.34  1997/07/16 20:04:48  curt
 * Minor tweaks to aid Win32 port.
 *
 * Revision 1.33  1997/07/12 03:50:20  curt
 * Added an #include <Windows32/Base.h> to help compiling for Win32
 *
 * Revision 1.32  1997/07/11 03:23:18  curt
 * Solved some scenery display/orientation problems.  Still have a positioning
 * (or transformation?) problem.
 *
 * Revision 1.31  1997/07/11 01:29:58  curt
 * More tweaking of terrian floor.
 *
 * Revision 1.30  1997/07/10 04:26:37  curt
 * We now can interpolated ground elevation for any position in the grid.  We
 * can use this to enforce a "hard" ground.  We still need to enforce some
 * bounds checking so that we don't try to lookup data points outside the
 * grid data set.
 *
 * Revision 1.29  1997/07/09 21:31:12  curt
 * Working on making the ground "hard."
 *
 * Revision 1.28  1997/07/08 18:20:12  curt
 * Working on establishing a hard ground.
 *
 * Revision 1.27  1997/07/07 20:59:49  curt
 * Working on scenery transformations to enable us to fly fluidly over the
 * poles with no discontinuity/distortion in scenery.
 *
 * Revision 1.26  1997/07/05 20:43:34  curt
 * renamed mat3 directory to Math so we could add other math related routines.
 *
 * Revision 1.25  1997/06/29 21:19:17  curt
 * Working on scenery management system.
 *
 * Revision 1.24  1997/06/26 22:14:53  curt
 * Beginning work on a scenery management system.
 *
 * Revision 1.23  1997/06/26 19:08:33  curt
 * Restructuring make, adding automatic "make dep" support.
 *
 * Revision 1.22  1997/06/25 15:39:47  curt
 * Minor changes to compile with rsxnt/win32.
 *
 * Revision 1.21  1997/06/22 21:44:41  curt
 * Working on intergrating the VRML (subset) parser.
 *
 * Revision 1.20  1997/06/21 17:12:53  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.19  1997/06/18 04:10:31  curt
 * A couple more runway tweaks ...
 *
 * Revision 1.18  1997/06/18 02:21:24  curt
 * Hacked in a runway
 *
 * Revision 1.17  1997/06/17 16:51:58  curt
 * Timer interval stuff now uses gettimeofday() instead of ftime()
 *
 * Revision 1.16  1997/06/17 04:19:16  curt
 * More timer related tweaks with respect to view direction changes.
 *
 * Revision 1.15  1997/06/17 03:41:10  curt
 * Nonsignal based interval timing is now working.
 * This would be a good time to look at cleaning up the code structure a bit.
 *
 * Revision 1.14  1997/06/16 19:32:51  curt
 * Starting to add general timer support.
 *
 * Revision 1.13  1997/06/02 03:40:06  curt
 * A tiny bit more view tweaking.
 *
 * Revision 1.12  1997/06/02 03:01:38  curt
 * Working on views (side, front, back, transitions, etc.)
 *
 * Revision 1.11  1997/05/31 19:16:25  curt
 * Elevator trim added.
 *
 * Revision 1.10  1997/05/31 04:13:52  curt
 * WE CAN NOW FLY!!!
 *
 * Continuing work on the LaRCsim flight model integration.
 * Added some MSFS-like keyboard input handling.
 *
 * Revision 1.9  1997/05/30 19:27:01  curt
 * The LaRCsim flight model is starting to look like it is working.
 *
 * Revision 1.8  1997/05/30 03:54:10  curt
 * Made a bit more progress towards integrating the LaRCsim flight model.
 *
 * Revision 1.7  1997/05/29 22:39:49  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.6  1997/05/29 12:31:39  curt
 * Minor tweaks, moving towards general flight model integration.
 *
 * Revision 1.5  1997/05/29 02:33:23  curt
 * Updated to reflect changing interfaces in other "modules."
 *
 * Revision 1.4  1997/05/27 17:44:31  curt
 * Renamed & rearranged variables and routines.   Added some initial simple
 * timer/alarm routines so the flight model can be updated on a regular 
 * interval.
 *
 * Revision 1.3  1997/05/23 15:40:25  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.2  1997/05/23 00:35:12  curt
 * Trying to get fog to work ...
 *
 * Revision 1.1  1997/05/21 15:57:51  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.3  1997/05/19 18:22:42  curt
 * Parameter tweaking ... starting to stub in fog support.
 *
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:52  curt
 * Initial revision.
 *
 */
