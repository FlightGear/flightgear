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
#include <stdio.h>

#include "GLUTkey.h"
#include "fg_init.h"
#include "views.h"

#include "../constants.h"
#include "../general.h"

#include "../Aircraft/aircraft.h"
#include "../Cockpit/cockpit.h"
#include "../Joystick/joystick.h"
#include "../Math/fg_geodesy.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"
#include "../Scenery/mesh.h"
#include "../Scenery/scenery.h"
#include "../Time/fg_time.h"
#include "../Time/fg_timer.h"
#include "../Time/sunpos.h"
#include "../Weather/weather.h"


/* This is a record containing all the info for the aircraft currently
   being operated */
struct AIRCRAFT current_aircraft;

/* This is a record containing global housekeeping information */
struct GENERAL general;

/* This is a record containing current weather info */
struct WEATHER current_weather;

/* This is a record containing current view parameters */
struct VIEW current_view;

/* view parameters */
static GLfloat win_ratio = 1.0;

/* sun direction */
/* static GLfloat sun_vec[4] = {1.0, 0.0, 0.0, 0.0 }; */

/* if the 4th field is 0.0, this specifies a direction ... */
/* clear color (sky) */
static GLfloat fgClearColor[4] = {0.60, 0.60, 0.90, 1.0};
/* fog color */
static GLfloat fgFogColor[4] =   {0.65, 0.65, 0.85, 1.0};

/* temporary hack */
/* extern struct mesh *mesh_ptr; */
/* Function prototypes */
/* GLint fgSceneryCompile_OLD(); */
/* static void fgSceneryDraw_OLD(); */

/* pointer to scenery structure */
/* static GLint scenery, runway; */

double Simtime;

/* Another hack */
int use_signals = 0;

/* Yet another hack. This one used by the HUD code. Michele */
int show_hud;


/**************************************************************************
 * fgInitVisuals() -- Initialize various GL/view parameters
 **************************************************************************/

static void fgInitVisuals() {
    struct fgTIME *t;
    struct WEATHER *w;

    t = &cur_time_params;
    w = &current_weather;

    glEnable( GL_DEPTH_TEST );
    /* glFrontFace(GL_CW); */
    glEnable( GL_CULL_FACE );

    /* If enabled, normal vectors specified with glNormal are scaled
       to unit length after transformation.  See glNormal. */
    glEnable( GL_NORMALIZE );

    glLightfv( GL_LIGHT0, GL_POSITION, t->sun_vec );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glShadeModel( GL_FLAT ); /* glShadeModel( GL_SMOOTH ); */

    glEnable( GL_FOG );
    glFogi (GL_FOG_MODE, GL_LINEAR);
    /* glFogf (GL_FOG_START, 1.0); */
    glFogf (GL_FOG_END, w->visibility);
    glFogfv (GL_FOG_COLOR, fgFogColor);
    /* glFogf (GL_FOG_DENSITY, w->visibility); */
    /* glHint (GL_FOG_HINT, GL_FASTEST); */

    glClearColor(fgClearColor[0], fgClearColor[1], fgClearColor[2], 
		 fgClearColor[3]);
}


/**************************************************************************
 * Update the view volume, position, and orientation
 **************************************************************************/

static void fgUpdateViewParams() {
    struct FLIGHT *f;
    struct fgTIME *t;
    struct VIEW *v;
    double ambient, diffuse, sky;
    GLfloat color[4] = { 1.0, 1.0, 0.50, 1.0 };
    GLfloat amb[3], diff[3], fog[4], clear[4];

    f = &current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;

    fgViewUpdate(f, v);

    /* Tell GL we are about to modify the projection parameters */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0/win_ratio, 1.0, 200000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* set up our view volume */
    gluLookAt(v->view_pos.x, v->view_pos.y, v->view_pos.z,
	      v->view_pos.x + v->view_forward[0], 
	      v->view_pos.y + v->view_forward[1], 
	      v->view_pos.z + v->view_forward[2],
	      v->view_up[0], v->view_up[1], v->view_up[2]);

    /* set the sun position */
    glLightfv( GL_LIGHT0, GL_POSITION, t->sun_vec );

    /* calculate lighting parameters based on sun's relative angle to
     * local up */
    /* ya kind'a have to plot this to see the magic */
    ambient = 0.4 * 
	pow(2.4, -t->sun_angle*t->sun_angle*t->sun_angle*t->sun_angle / 3.0);
    diffuse = 0.4 * cos(0.6 * t->sun_angle * t->sun_angle);
    sky = 0.85 * 
	pow(1.6, -t->sun_angle*t->sun_angle*t->sun_angle*t->sun_angle/2.0) 
	+ 0.15;

    if ( ambient < 0.1 ) { ambient = 0.1; }
    if ( diffuse < 0.0 ) { diffuse = 0.0; }

    if ( sky < 0.0 ) { sky = 0.0; }

    amb[0] = color[0] * ambient;
    amb[1] = color[1] * ambient;
    amb[2] = color[2] * ambient;

    diff[0] = color[0] * diffuse;
    diff[1] = color[1] * diffuse;
    diff[2] = color[2] * diffuse;

    /* set lighting parameters */
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb );
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff );

    /* set fog color */
    fog[0] = fgFogColor[0] * (ambient + diffuse);
    fog[1] = fgFogColor[1] * (ambient + diffuse);
    fog[2] = fgFogColor[2] * (ambient + diffuse);
    fog[3] = fgFogColor[3];
    glFogfv (GL_FOG_COLOR, fog);

    /* set sky color */
    clear[0] = fgClearColor[0] * sky;
    clear[1] = fgClearColor[1] * sky;
    clear[2] = fgClearColor[2] * sky;
    clear[3] = fgClearColor[3];
    glClearColor(clear[0], clear[1], clear[2], clear[3]);
}


/**************************************************************************
 * Update all Visuals (redraws anything graphics related)
 **************************************************************************/

static void fgUpdateVisuals( void ) {
    /* update view volume parameters */
    fgUpdateViewParams();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* Tell GL we are switching to model view parameters */
    glMatrixMode(GL_MODELVIEW);
    /* glLoadIdentity(); */

    /* draw scenery */
    fgSceneryRender();

    /* display HUD */
    if( show_hud )
     	fgCockpitUpdate();

    #ifdef GLUT
      glutSwapBuffers();
    #endif
}


/**************************************************************************
 * Update internal time dependent calculations (i.e. flight model)
 **************************************************************************/

void fgUpdateTimeDepCalcs(int multi_loop) {
    struct FLIGHT *f;
    struct fgTIME *t;
    struct VIEW *v;
    int i;

    f = &current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;

    /* update the flight model */
    if ( multi_loop < 0 ) {
	multi_loop = DEFAULT_MULTILOOP;
    }

    /* printf("updating flight model x %d\n", multi_loop); */
    fgFlightModelUpdate(FG_LARCSIM, f, multi_loop);

    /* refresh shared sun position and sun_vec */
    fgUpdateSunPos(scenery.center);

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


void fgInitTimeDepCalcs() {
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

    runway = glGenLists(1);
    glNewList(runway, GL_COMPILE);
    */
    /* draw concrete */
/*    glBegin(GL_POLYGON);
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, concrete );
    glNormal3f(0.0, 0.0, 1.0);

    glVertex3d( 0.0,   -width/2.0, 0.0);
    glVertex3d( 0.0,    width/2.0, 0.0);
    glVertex3d(length,  width/2.0, 0.0);
    glVertex3d(length, -width/2.0, 0.0);
    glEnd();
    */
    /* draw center line */
/*    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, line );
    line_len = length / ( 2 * num_lines + 1);
    printf("line_len = %.3f\n", line_len);
    line_width_2 = 0.02;
    cur_pos = line_len;
    for ( i = 0; i < num_lines; i++ ) {
	glBegin(GL_POLYGON);
	glVertex3d( cur_pos, -line_width_2, 0.005);
	glVertex3d( cur_pos,  line_width_2, 0.005);
	cur_pos += line_len;
	glVertex3d( cur_pos,  line_width_2, 0.005);
	glVertex3d( cur_pos, -line_width_2, 0.005);
	cur_pos += line_len;
	glEnd();
    }

    glEndList();

    return(runway);
}
*/

/* draw the scenery */
/*static void fgSceneryDraw_OLD() {
    static float z = 32.35;

    glPushMatrix();

    glCallList(scenery);

    printf("*** Drawing runway at %.2f\n", z);

    glTranslatef( -398391.28, 120070.41, 32.35);
    glRotatef(170.0, 0.0, 0.0, 1.0);
    glCallList(runway);

    glPopMatrix();
}
*/

/* What should we do when we have nothing else to do?  How about get
 * ready for the next move and update the display? */
static void fgMainLoop( void ) {
    static int remainder = 0;
    int elapsed, multi_loop;
    double cur_elev;
    double joy_x, joy_y;
    int joy_b1, joy_b2;
    struct FLIGHT *f;

    f = &current_aircraft.flight;

    /* Read joystick */
    /* fgJoystickRead( &joy_x, &joy_y, &joy_b1, &joy_b2 );
    printf( "Joystick X %f  Y %f  B1 %d  B2 %d\n",  
	    joy_x, joy_y, joy_b1, joy_b2 );
    fgElevSet( -joy_y );
    fgAileronSet( joy_x ); */

    /* update the weather for our current position */
    fgWeatherUpdate(FG_Longitude * RAD_TO_ARCSEC, 
		    FG_Latitude * RAD_TO_ARCSEC, 
		    FG_Altitude * FEET_TO_METER);

    /* Calculate model iterations needed */
    elapsed = fgGetTimeInterval();
    printf("Time interval is = %d, previous remainder is = %d\n", elapsed, 
	   remainder);
    printf("--> Frame rate is = %.2f\n", 1000.0 / (float)elapsed);
    elapsed += remainder;

    multi_loop = ((float)elapsed * 0.001) * DEFAULT_MODEL_HZ;
    remainder = elapsed - ((multi_loop*1000) / DEFAULT_MODEL_HZ);
    printf("Model iterations needed = %d, new remainder = %d\n", multi_loop, 
	   remainder);

    if ( ! use_signals ) {
	/* flight model */
	fgUpdateTimeDepCalcs(multi_loop);
    }

    /* I'm just sticking this here for now, it should probably move 
     * eventually */
    cur_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC, 
			       FG_Latitude  * RAD_TO_ARCSEC);
    printf("Ground elevation is %.2f meters here.\n", cur_elev);
    /* FG_Runway_altitude = cur_elev * METER_TO_FEET; */

    if ( FG_Altitude * FEET_TO_METER < cur_elev + 3.758099) {
	/* set this here, otherwise if we set runway height above our
	   current height we get a really nasty bounce. */
	FG_Runway_altitude = FG_Altitude - 3.758099;

	/* now set aircraft altitude above ground */
	FG_Altitude = cur_elev * METER_TO_FEET + 3.758099;
	printf("<*> resetting altitude to %.0f meters\n", 
	       FG_Altitude * FEET_TO_METER);
    }

    aircraft_debug(1);

    /* redraw display */
    fgUpdateVisuals();
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

    /* Inform gl of our view window size */
    glViewport(0, 0, (GLint)width, (GLint)height);

    fgUpdateViewParams();
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


/**************************************************************************
 * Main ...
 **************************************************************************/

int main( int argc, char *argv[] ) {
    struct FLIGHT *f;

    f = &current_aircraft.flight;

    printf("Flight Gear: prototype version %s\n\n", VERSION);

    /**********************************************************************
     * Initialize the Window/Graphics environment.
     **********************************************************************/

    #ifdef GLUT
      /* initialize GLUT */
      glutInit(&argc, argv);

      /* Define Display Parameters */
      glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

      /* Define initial window size */
      glutInitWindowSize(640, 480);

      /* Initialize windows */
      glutCreateWindow("Flight Gear");
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
      glutReshapeFunc( fgReshape );

      /* call key() on keyboard event */
      glutKeyboardFunc( GLUTkey );
      glutSpecialFunc( GLUTspecialkey );

      /* call fgMainLoop() whenever there is nothing else to do */
      glutIdleFunc( fgMainLoop );

      /* draw the scene */
      glutDisplayFunc( fgUpdateVisuals );

      /* pass control off to the GLUT event handler */
      glutMainLoop();
    #endif

    return(0);
}


#ifdef NO_PRINTF
  #include <stdarg.h>
  int printf (const char *format, ...) {
  }
#endif


/* $Log$
/* Revision 1.15  1997/09/05 14:17:27  curt
/* More tweaking with stars.
/*
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
