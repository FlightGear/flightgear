/**************************************************************************
 * GLmain.c -- top level sim routines
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


#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef GLUT
    #include <GL/glut.h>
    #include "GLUTkey.h"
#elif MESA_TK
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
    #include "GLTKkey.h"
#endif

#include "../Aircraft/aircraft.h"
#include "../Scenery/scenery.h"
#include "../Math/mat3.h"
#include "../Timer/fg_timer.h"


#define DEG_TO_RAD       0.017453292
#define RAD_TO_DEG       57.29577951

#ifndef M_PI
#define M_PI        3.14159265358979323846	/* pi */
#endif

#ifndef PI2                                     
#define PI2  (M_PI + M_PI)
#endif                                                           


#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923	/* pi/2 */
#endif

/* This is a record containing all the info for the aircraft currently
   being operated */
struct aircraft_params current_aircraft;

/* view parameters */
static GLfloat win_ratio = 1.0;

/* sun direction */
static GLfloat sun_vec[4] = {-3.0, 1.0, 2.0, 0.0 };

/* temporary hack */
/* extern struct mesh *mesh_ptr; */
/* Function prototypes */
/* GLint fgSceneryCompile_OLD(); */
/* static void fgSceneryDraw_OLD(); */

/* pointer to scenery structure */
/* static GLint scenery, runway; */

/* Another hack */
double fogDensity = 2000.0;
double view_offset = 0.0;
double goal_view_offset = 0.0;

/* Another hack */
#define DEFAULT_TIMER_HZ 20
#define DEFAULT_MULTILOOP 6
#define DEFAULT_MODEL_HZ (DEFAULT_TIMER_HZ * DEFAULT_MULTILOOP)

double Simtime;

/* Another hack */
int use_signals = 0;


/**************************************************************************
 * fgInitVisuals() -- Initialize various GL/view parameters
 **************************************************************************/

static void fgInitVisuals() {
    /* if the 4th field is 0.0, this specifies a direction ... */
    static GLfloat fogColor[4] = {0.65, 0.65, 0.85, 1.0};
    
    glEnable( GL_DEPTH_TEST );
    glFrontFace(GL_CW);
    glEnable( GL_CULL_FACE );

    /* If enabled, normal vectors specified with glNormal are scaled
       to unit length after transformation.  See glNormal. */
    glEnable( GL_NORMALIZE );

    glLightfv( GL_LIGHT0, GL_POSITION, sun_vec );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glShadeModel( GL_FLAT ); /* glShadeModel( GL_SMOOTH ); */

    glEnable( GL_FOG );
    glFogi (GL_FOG_MODE, GL_LINEAR);
    /* glFogf (GL_FOG_START, 1.0); */
    glFogf (GL_FOG_END, fogDensity);
    glFogfv (GL_FOG_COLOR, fogColor);
    /* glFogf (GL_FOG_DENSITY, fogDensity); */
    /* glHint (GL_FOG_HINT, GL_FASTEST); */

    glClearColor(0.6, 0.6, 0.9, 1.0);
}


/**************************************************************************
 * Update the view volume, position, and orientation
 **************************************************************************/

static void fgUpdateViewParams() {
    double pos_x, pos_y, pos_z;
    struct flight_params *f;
    MAT3mat R, TMP;
    MAT3vec vec, up, forward, fwrd_view;

    f = &current_aircraft.flight;

    /* Tell GL we are about to modify the projection parameters */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0/win_ratio, 0.01, 6000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* calculate position in arc seconds */
    pos_x = (FG_Longitude * RAD_TO_DEG) * 3600.0;
    pos_y = (FG_Latitude   * RAD_TO_DEG) * 3600.0;
    pos_z = FG_Altitude * 0.01; /* (Convert feet to aproximate arcsecs) */

    printf("*** pos_z = %.2f\n", pos_z);

    /* build current rotation matrix */
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3rotate(R, vec, FG_Phi);
    /* printf("Roll matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    /* MAT3mult_vec(vec, vec, R); */
    MAT3rotate(TMP, vec, -FG_Theta);
    /* printf("Pitch matrix\n"); */
    /* MAT3print(TMP, stdout); */
    MAT3mult(R, R, TMP);

    MAT3_SET_VEC(vec, 0.0, 0.0, -1.0);
    /* MAT3mult_vec(vec, vec, R); */
    /* MAT3rotate(TMP, vec, M_PI + M_PI_2 + FG_Psi + view_offset); */
    MAT3rotate(TMP, vec, FG_Psi - M_PI_2);
    /* printf("Yaw matrix\n");
    MAT3print(TMP, stdout); */
    MAT3mult(R, R, TMP);

    /* MAT3print(R, stdout); */

    /* generate the current up, forward, and fwrd-view vectors */
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(up, vec, R);

    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(forward, vec, R);
    printf("Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
	   forward[2]);

    MAT3rotate(TMP, up, view_offset);
    MAT3mult_vec(fwrd_view, forward, TMP);

    gluLookAt(pos_x, pos_y, pos_z,
	      pos_x + fwrd_view[0], pos_y + fwrd_view[1], pos_z + fwrd_view[2],
	      up[0], up[1], up[2]);

    glLightfv( GL_LIGHT0, GL_POSITION, sun_vec );
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

    #ifdef GLUT
      glutSwapBuffers();
    #elif MESA_TK
      tkSwapBuffers();
    #endif
}


/**************************************************************************
 * Update internal time dependent calculations (i.e. flight model)
 **************************************************************************/

void fgUpdateTimeDepCalcs(int multi_loop) {
    struct flight_params *f;
    int i;

    f = &current_aircraft.flight;

    /* update the flight model */
    if ( multi_loop < 0 ) {
	multi_loop = DEFAULT_MULTILOOP;
    }

    /* printf("updating flight model x %d\n", multi_loop); */
    fgFlightModelUpdate(FG_LARCSIM, f, multi_loop);

    for ( i = 0; i < multi_loop; i++ ) {
	if ( fabs(goal_view_offset - view_offset) < 0.05 ) {
	    view_offset = goal_view_offset;
	    break;
	} else {
	    /* move view_offset towards goal_view_offset */
	    if ( goal_view_offset > view_offset ) {
		if ( goal_view_offset - view_offset < M_PI ) {
		    view_offset += 0.01;
		} else {
		    view_offset -= 0.01;
		}
	    } else {
		if ( view_offset - goal_view_offset < M_PI ) {
		    view_offset -= 0.01;
		} else {
		    view_offset += 0.01;
		}
	    }
	    if ( view_offset > PI2 ) {
		view_offset -= PI2;
	    } else if ( view_offset < 0 ) {
		view_offset += PI2;
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

    elapsed = fgGetTimeInterval();
    printf("Time interval is = %d, previous remainder is = %d\n", elapsed, 
	   remainder);
    printf("--> Frame rate is = %.2f\n", 1000.0 / (float)elapsed);
    elapsed += remainder;

    multi_loop = ((float)elapsed * 0.001) * DEFAULT_MODEL_HZ;
    remainder = elapsed - ((multi_loop*1000) / DEFAULT_MODEL_HZ);
    printf("Model iterations needed = %d, new remainder = %d\n", multi_loop, 
	   remainder);

    aircraft_debug(1);
    fgUpdateVisuals();

    if ( ! use_signals ) {
	fgUpdateTimeDepCalcs(multi_loop);
    }
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
    struct flight_params *f;

    f = &current_aircraft.flight;

    printf("Flight Gear: prototype code to test OpenGL, LaRCsim, and VRML\n\n");


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

      /* Initialize the main window */
      glutCreateWindow("Flight Gear");
    #elif MESA_TK
      /* Define initial window size */
      tkInitPosition(0, 0, 640, 480);

      /* Define Display Parameters */
      tkInitDisplayMode( TK_RGB | TK_DEPTH | TK_DOUBLE | TK_DIRECT );

      /* Initialize the main window */
      if (tkInitWindow("Flight Gear") == GL_FALSE) {
	  tkQuit();
      }
    #endif

    /* setup view parameters, only makes GL calls */
    fgInitVisuals();


    /****************************************************************
     * The following section sets up the flight model EOM parameters and 
     * should really be read in from one or more files.
     ****************************************************************/

    /* Globe Aiport, AZ */
    FG_Runway_altitude = 3234.5;
    FG_Runway_latitude = 120070.41;
    FG_Runway_longitude = -398391.28;
    FG_Runway_heading = 102.0 * DEG_TO_RAD;

    /* Initial Position */
    /* FG_Latitude  = (  120070.41 / 3600.0 ) * DEG_TO_RAD;
    FG_Longitude = ( -398391.28 / 3600.0 ) * DEG_TO_RAD; */
    FG_Latitude  = 0.0;
    FG_Longitude = 0.0;
    FG_Altitude  = FG_Runway_altitude + 3.758099;

    printf("Initial position is: (%.4f, %.4f, %.2f)\n", FG_Latitude, 
	   FG_Longitude, FG_Altitude);

      /* Initial Velocity */
    FG_V_north = 0.0 /*  7.287719E+00 */;
    FG_V_east  = 0.0 /*  1.521770E+03 */;
    FG_V_down  = 0.0 /* -1.265722E-05 */;

    /* Initial Orientation */
    FG_Phi   = -2.658474E-06;
    FG_Theta =  7.401790E-03;
    FG_Psi   =  282.0 * DEG_TO_RAD;

    /* Initial Angular B rates */
    FG_P_body = 7.206685E-05;
    FG_Q_body = 0.000000E+00;
    FG_R_body = 9.492658E-05;

    FG_Earth_position_angle = 0.000000E+00;

    /* Mass properties and geometry values */
    FG_Mass = 8.547270E+01;
    FG_I_xx = 1.048000E+03;
    FG_I_yy = 3.000000E+03;
    FG_I_zz = 3.530000E+03;
    FG_I_xz = 0.000000E+00;

    /* CG position w.r.t. ref. point */
    FG_Dx_cg = 0.000000E+00;
    FG_Dy_cg = 0.000000E+00;
    FG_Dz_cg = 0.000000E+00;

    /* Set initial position and slew parameters */
    /* fgSlewInit(-398391.3, 120070.41, 244, 3.1415); */ /* GLOBE Airport */
    /* fgSlewInit(-335340,162540, 15, 4.38); */
    /* fgSlewInit(-398673.28,120625.64, 53, 4.38); */

    /* Initialize the flight model data structures base on above values */
    fgFlightModelInit( FG_LARCSIM, f, 1.0 / DEFAULT_MODEL_HZ );

    if ( use_signals ) {
	/* init timer routines, signals, etc.  Arrange for an alarm
	   signal to be generated, etc. */
	fgInitTimeDepCalcs();
    }


    /**********************************************************************
     * The following section (and the functions elsewhere in this file) 
     * set up the scenery management system. This part is a big hack, 
     * and needs to be moved to it's own area.
     **********************************************************************/

    /* Initialize the Scenery Management system */
    fgSceneryInit();

    /* Tell the Scenery Management system where we are so it can load
     * the correct scenery data */
    fgSceneryUpdate(FG_Latitude, FG_Longitude, FG_Altitude);


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
    #elif MESA_TK
      /* call fgReshape() on expose events */
      tkExposeFunc( fgReshape );

      /* call fgReshape() on window resizes */
      tkReshapeFunc( fgReshape );

      /* call key() on keyboard event */
      tkKeyDownFunc( GLTKkey );

      /* call fgMainLoop() whenever there is nothing else to do */
      tkIdleFunc( fgMainLoop );

      /* draw the scene */
      tkDisplayFunc( fgUpdateVisuals );

      /* pass control off to the tk event handler */
      tkExec();
    #endif

    return(0);
}


/* $Log$
/* Revision 1.26  1997/07/05 20:43:34  curt
/* renamed mat3 directory to Math so we could add other math related routines.
/*
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
