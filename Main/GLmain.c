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
#include <signal.h>    /* for timer routines */

#if defined(__WATCOMC__) || defined(__MSC__)
#  include <time.h>
#else
#  include <sys/time.h>
#endif

#ifdef GLUT
    #include <GL/glut.h>
    #include "GLUTkey.h"
#elif MESA_TK
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
    #include "GLTKkey.h"
#endif

#include "../aircraft/aircraft.h"
#include "../scenery/scenery.h"
#include "../mat3/mat3.h"


#define DEG_TO_RAD       0.017453292
#define RAD_TO_DEG       57.29577951

/* This is a record containing all the info for the aircraft currently
   being operated */
struct aircraft_params current_aircraft;

/* view parameters */
static GLfloat win_ratio = 1.0;

/* sun direction */
static GLfloat sun_vec[4] = {-3.0, 1.0, 2.0, 0.0 };

/* temporary hack */
extern struct mesh *mesh_ptr;
/* Function prototypes */
GLint fgSceneryCompile();
static void fgSceneryDraw();
/* pointer to terrain mesh structure */
static GLint mesh;

/* Another hack */
double fogDensity = 2000.0;

/* Another hack */
#define DEFAULT_MODEL_HZ 20
#define DEFAULT_MULTILOOP 6
double Simtime;


/**************************************************************************
 * fgInitVisuals() -- Initialize various GL/view parameters
 **************************************************************************/

static void fgInitVisuals() {
    /* if the 4th field is 0.0, this specifies a direction ... */
    static GLfloat color[4] = { 0.3, 0.7, 0.2, 1.0 };
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

    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color );
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
    MAT3mat R, tmp;
    MAT3vec vec, forward, up;

    f = &current_aircraft.flight;

    /* Tell GL we are about to modify the projection parameters */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0/win_ratio, 1.0, 6000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* calculate position in arc seconds */
    pos_x = (FG_Longitude * RAD_TO_DEG) * 3600.0;
    pos_y = (FG_Latitude   * RAD_TO_DEG) * 3600.0;
    pos_z = FG_Altitude * 0.01; /* (Convert feet to aproximate arcsecs) */

    /* build current rotation matrix */
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3rotate(R, vec, FG_Phi);
    /* printf("Roll matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, -1.0, 0.0);
    MAT3rotate(tmp, vec, FG_Theta);
    /* printf("Pitch matrix\n"); */
    /* MAT3print(tmp, stdout); */
    MAT3mult(R, R, tmp);

    MAT3_SET_VEC(vec, 0.0, 0.0, -1.0);
    MAT3rotate(tmp, vec, M_PI + M_PI_2 + FG_Psi );
    /* printf("Yaw matrix\n");
    MAT3print(tmp, stdout); */
    MAT3mult(R, R, tmp);

    /* MAT3print(R, stdout); */

    /* generate the current forward and up vectors */
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(forward, vec, R);
    printf("Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
	   forward[2]);
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(up, vec, R);

    gluLookAt(pos_x, pos_y, pos_z,
	      pos_x + forward[0], pos_y + forward[1], pos_z + forward[2],
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

    /* draw terrain mesh */
    fgSceneryDraw();

    #ifdef GLUT
      glutSwapBuffers();
    #elif MESA_TK
      tkSwapBuffers();
    #endif
}


/**************************************************************************
 * Timer management routines
 **************************************************************************/

static struct itimerval t, ot;

/* This routine catches the SIGALRM */
void fgTimerCatch() {
    struct flight_params *f;
    static double lastSimtime = -99.9;
    int Overrun;

    /* ignore any SIGALRM's until we come back from our EOM iteration */
    signal(SIGALRM, SIG_IGN);

    f = &current_aircraft.flight;

    /* printf("In fgTimerCatch()\n"); */

    Overrun = (lastSimtime == Simtime);

    /* add this back in when you get simtime working */
    /* if ( Overrun ) {
	printf("OVERRUN!!!\n");
    } */

    /* update the flight model */
    fgFlightModelUpdate(FG_LARCSIM, f, DEFAULT_MULTILOOP);

    lastSimtime = Simtime;
    signal(SIGALRM, fgTimerCatch);
}

/* this routine initializes the interval timer to generate a SIGALRM after
 * the specified interval (dt) */
void fgTimerInit(float dt) {
    int terr;
    int isec;
    float usec;

    isec = (int) dt;
    usec = 1000000* (dt - (float) isec);

    t.it_interval.tv_sec = isec;
    t.it_interval.tv_usec = usec;
    t.it_value.tv_sec = isec;
    t.it_value.tv_usec = usec;
    /* printf("fgTimerInit() called\n"); */
    fgTimerCatch();   /* set up for SIGALRM signal catch */
    terr = setitimer( ITIMER_REAL, &t, &ot );
    if (terr) perror("Error returned from setitimer");
}


/**************************************************************************
 * Scenery management routines
 **************************************************************************/

static void fgSceneryInit() {
    /* make terrain mesh */
    mesh = fgSceneryCompile();
}


/* create the terrain mesh */
GLint fgSceneryCompile() {
    GLint mesh;

    mesh = mesh2GL(mesh_ptr);

    return(mesh);
}


/* draw the terrain mesh */
static void fgSceneryDraw() {
    glCallList(mesh);
}


/* What should we do when we have nothing else to do?  How about get
 * ready for the next move?*/
static void fgMainLoop( void )
{
    fgSlewUpdate();
    aircraft_debug(1);

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
    struct flight_params *f;

    f = &current_aircraft.flight;

    /* parse the scenery file */
    parse_scenery(argv[1]);

    #ifdef GLUT
      /* initialize GLUT */
      glutInit(&argc, argv);

      /* Define Display Parameters */
      glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

      /* Define initial window size */
      glutInitWindowSize(640, 480);

      /* Initialize the main window */
      glutCreateWindow("Terrain Demo");
    #elif MESA_TK
      /* Define initial window size */
      tkInitPosition(0, 0, 640, 480);

      /* Define Display Parameters */
      tkInitDisplayMode( TK_RGB | TK_DEPTH | TK_DOUBLE | TK_DIRECT );

      /* Initialize the main window */
      if (tkInitWindow("Terrain Demo") == GL_FALSE) {
	  tkQuit();
      }
    #endif

    /* setup view parameters, only makes GL calls */
    fgInitVisuals();

    /* Globe Aiport, AZ */
    FG_Runway_altitude = 3234.5;
    FG_Runway_latitude = 120070.41;
    FG_Runway_longitude = -398391.28;
    FG_Runway_heading = 102.0 * DEG_TO_RAD;

    /* Initial Position */
    FG_Latitude  = (  120070.41 / 3600.0 ) * DEG_TO_RAD;
    FG_Longitude = ( -398391.28 / 3600.0 ) * DEG_TO_RAD;
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
    FG_Psi   =  102.0 * DEG_TO_RAD;

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

    fgFlightModelInit(FG_LARCSIM, f, 1.0/(DEFAULT_MODEL_HZ*DEFAULT_MULTILOOP));

    /* build all objects */
    fgSceneryInit();

    /* initialize timer */
    fgTimerInit( 1.0 / DEFAULT_MODEL_HZ );

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
/* Revision 1.11  1997/05/31 19:16:25  curt
/* Elevator trim added.
/*
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
