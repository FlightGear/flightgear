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


#include <stdio.h>

#ifdef WIN32
#  include <windows.h>                     
#endif

#ifdef GLUT
    #include <GL/glut.h>
    #include "GLUTkey.h"
#elif MESA_TK
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
    #include "GLTKkey.h"
#endif

#include "../constants.h"

#include "../Aircraft/aircraft.h"
#include "../Scenery/mesh.h"
#include "../Scenery/scenery.h"
#include "../Math/fg_random.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"
#include "../Time/fg_timer.h"
#include "../Weather/weather.h"


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
double fogDensity = 60000.0; /* in meters */
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
    /* glFrontFace(GL_CW); */
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
    struct fgCartesianPoint view_pos /*, alt_up */;
    struct flight_params *f;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    MAT3vec vec, view_up, forward, view_forward, local_up;

    f = &current_aircraft.flight;

    /* Tell GL we are about to modify the projection parameters */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0/win_ratio, 1.0, 200000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* calculate view position in current FG view coordinate system */
    view_pos = fgPolarToCart(FG_Longitude, FG_Lat_geocentric, 
			     FG_Radius_to_vehicle * FEET_TO_METER + 1.0);
    printf("View pos = %.4f, %.4f, %.4f\n", view_pos.x, view_pos.y, view_pos.z);

    /* Derive the LOCAL aircraft rotation matrix (roll, pitch, yaw) */
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, FG_Phi);
    /* printf("Roll matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    /* MAT3mult_vec(vec, vec, R); */
    MAT3rotate(TMP, vec, FG_Theta);
    /* printf("Pitch matrix\n"); */
    /* MAT3print(TMP, stdout); */
    MAT3mult(R, R, TMP);

    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    /* MAT3mult_vec(vec, vec, R); */
    /* MAT3rotate(TMP, vec, FG_Psi - FG_PI_2); */
    MAT3rotate(TMP, vec, -FG_Psi);
    /* printf("Yaw matrix\n");
    MAT3print(TMP, stdout); */
    MAT3mult(LOCAL, R, TMP);
    /* printf("LOCAL matrix\n"); */
    /* MAT3print(LOCAL, stdout); */

    /* Derive the local UP transformation matrix based on *geodetic*
     * coordinates */
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, FG_Longitude);     /* R = rotate about Z axis */
    /* printf("Longitude matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3mult_vec(vec, vec, R);
    MAT3rotate(TMP, vec, -FG_Latitude);  /* TMP = rotate about X axis */
    /* printf("Latitude matrix\n"); */
    /* MAT3print(TMP, stdout); */

    MAT3mult(UP, R, TMP);
    /* printf("Local up matrix\n"); */
    /* MAT3print(UP, stdout); */

    MAT3_SET_VEC(local_up, 1.0, 0.0, 0.0);
    MAT3mult_vec(local_up, local_up, UP);

    printf("    Local Up = (%.4f, %.4f, %.4f)\n", local_up[0], local_up[1], 
	   local_up[2]);
    
    /* Alternative method to Derive local up vector based on
     * *geodetic* coordinates */
    /* alt_up = fgPolarToCart(FG_Longitude, FG_Latitude, 1.0); */
    /* printf("    Alt Up = (%.4f, %.4f, %.4f)\n", 
       alt_up.x, alt_up.y, alt_up.z); */

    /* Derive the VIEW matrix */
    MAT3mult(VIEW, LOCAL, UP);
    /* printf("VIEW matrix\n"); */
    /* MAT3print(VIEW, stdout); */

    /* generate the current up, forward, and fwrd-view vectors */
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(view_up, vec, VIEW);

    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(forward, vec, VIEW);
    printf("Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
	   forward[2]);

    MAT3rotate(TMP, view_up, view_offset);
    MAT3mult_vec(view_forward, forward, TMP);

    gluLookAt(view_pos.x, view_pos.y, view_pos.z,
	      view_pos.x + view_forward[0], view_pos.y + view_forward[1], 
	      view_pos.z + view_forward[2],
	      view_up[0], view_up[1], view_up[2]);

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
		if ( goal_view_offset - view_offset < FG_PI ) {
		    view_offset += 0.01;
		} else {
		    view_offset -= 0.01;
		}
	    } else {
		if ( view_offset - goal_view_offset < FG_PI ) {
		    view_offset -= 0.01;
		} else {
		    view_offset += 0.01;
		}
	    }
	    if ( view_offset > FG_2PI ) {
		view_offset -= FG_2PI;
	    } else if ( view_offset < 0 ) {
		view_offset += FG_2PI;
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
    double rough_elev;
    struct flight_params *f;

    f = &current_aircraft.flight;

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

    /* I'm just sticking this here for now, it should probably move 
     * eventually */
    rough_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC, 
			       FG_Latitude  * RAD_TO_ARCSEC);
    printf("Ground elevation is %.2f meters here.\n", rough_elev);
    /* FG_Runway_altitude = rough_elev * METER_TO_FEET; */

    if ( FG_Altitude * FEET_TO_METER < rough_elev + 3.758099) {
	/* set this here, otherwise if we set runway height above our
	   current height we get a really nasty bounce. */
	FG_Runway_altitude = FG_Altitude - 3.758099;

	/* now set aircraft altitude above ground */
	FG_Altitude = rough_elev * METER_TO_FEET + 3.758099;
	printf("<*> resetting altitude to %.0f meters\n", 
	       FG_Altitude * FEET_TO_METER);
    }

    /* update the weather for our current position */
    fgWeatherUpdate(FG_Longitude * RAD_TO_ARCSEC, 
		    FG_Latitude * RAD_TO_ARCSEC, 
		    FG_Altitude * FEET_TO_METER);
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
    double rough_elev;

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

    /* seed the random number generater */
    fg_srandom();

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
    FG_Latitude  = (  120070.41 / 3600.0 ) * DEG_TO_RAD;
    FG_Longitude = ( -398391.28 / 3600.0 ) * DEG_TO_RAD;
    FG_Altitude = FG_Runway_altitude + 3.758099;

    /* FG_Latitude  = 0.0; */
    /* FG_Longitude = 0.0; */
    /* FG_Altitude = 15000.0; */

    printf("Initial position is: (%.4f, %.4f, %.2f)\n", FG_Latitude, 
	   FG_Longitude, FG_Altitude);

      /* Initial Velocity */
    FG_V_north = 0.0 /*  7.287719E+00 */;
    FG_V_east  = 0.0 /*  1.521770E+03 */;
    FG_V_down  = 0.0 /* -1.265722E-05 */;

    /* Initial Orientation */
    FG_Phi   = -2.658474E-06;
    FG_Theta =  7.401790E-03;
    /* FG_Psi   =  270.0 * DEG_TO_RAD; */
    FG_Psi   =  0.0 * DEG_TO_RAD;

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

   /* Initialize the Scenery Management system */
    fgSceneryInit();

    /* Tell the Scenery Management system where we are so it can load
     * the correct scenery data */
    fgSceneryUpdate(FG_Latitude, FG_Longitude, FG_Altitude);

    /* I'm just sticking this here for now, it should probably move 
     * eventually */
    rough_elev = mesh_altitude(FG_Longitude * RAD_TO_DEG * 3600.0, 
			       FG_Latitude  * RAD_TO_DEG * 3600.0);
    printf("Ground elevation is %.2f meters here.\n", rough_elev);
    if ( rough_elev > -9990.0 ) {
	FG_Runway_altitude = rough_elev * METER_TO_FEET;
    }

    if ( FG_Altitude < FG_Runway_altitude ) {
	FG_Altitude = FG_Runway_altitude + 3.758099;
    }
    /* end of thing that I just stuck in that I should probably move */

    /* Initialize the flight model data structures base on above values */
    fgFlightModelInit( FG_LARCSIM, f, 1.0 / DEFAULT_MODEL_HZ );

    if ( use_signals ) {
	/* init timer routines, signals, etc.  Arrange for an alarm
	   signal to be generated, etc. */
	fgInitTimeDepCalcs();
    }

    /* Initialize the weather modeling subsystem */
    fgWeatherInit();

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

#ifdef NO_PRINTF

#include <stdarg.h>
int printf (const char *format, ...) {
}

#endif


/* $Log$
/* Revision 1.42  1997/08/01 19:43:33  curt
/* Making progress with coordinate system overhaul.
/*
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
