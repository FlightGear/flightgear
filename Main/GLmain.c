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
#include <sys/time.h>

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


/* This is a record containing all the info for the aircraft currently
   being operated */
struct aircraft_params current_aircraft;

/* temporary hack */
extern struct mesh *mesh_ptr;

/* Function prototypes */
GLint make_mesh();
static void draw_mesh();


/* view parameters */
static GLfloat win_ratio = 1.0;

/* pointer to terrain mesh structure */
static GLint mesh;

double fogDensity = 0.001;

/* init_view() -- Setup view parameters */
static void init_view() {
    /* if the 4th field is 0.0, this specifies a direction ... */
    static GLfloat pos[4] = {-3.0, 1.0, 3.0, 0.0 };
    static GLfloat color[4] = { 0.3, 0.7, 0.2, 1.0 };
    static GLfloat fogColor[4] = {0.5, 0.5, 0.5, 1.0};
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    /* If enabled, normal vectors specified with glNormal are scaled
       to unit length after transformation.  See glNormal. */
    glEnable( GL_NORMALIZE );

    glLightfv( GL_LIGHT0, GL_POSITION, pos );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color );
    glShadeModel( GL_FLAT ); /* glShadeModel( GL_SMOOTH ); */

    glEnable( GL_FOG );
    glFogi (GL_FOG_MODE, GL_LINEAR);
    /* glFogf (GL_FOG_START, 1.0); */
    glFogf (GL_FOG_END, 2000.0);
    glFogfv (GL_FOG_COLOR, fogColor);
    /* glFogf (GL_FOG_DENSITY, fogDensity); */
    /* glHint (GL_FOG_HINT, GL_FASTEST); */

    glClearColor(0.6, 0.6, 0.9, 1.0);
}


/* init_scene() -- build all objects */
static void init_scene() {

    /* make terrain mesh */
    mesh = make_mesh();
}


/* create the terrain mesh */
GLint make_mesh() {
    GLint mesh;

    mesh = mesh2GL(mesh_ptr);

    return(mesh);
}


/* update the view volume */
static void update_view() {
    struct flight_params *f;

    f = &current_aircraft.flight;

    /* Tell GL we are about to modify the projection parameters */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0/win_ratio, 1.0, 6000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(f->pos_x, f->pos_y, f->pos_z,
	      f->pos_x + cos(f->Psi), f->pos_y + sin(f->Psi), f->pos_z,
	      0.0, 0.0, 1.0);
}


/* draw the scene */
static void draw_scene( void ) {
    /* update view volume parameters */
    update_view();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* Tell GL we are switching to model view parameters */
    glMatrixMode(GL_MODELVIEW);
    /* glLoadIdentity(); */

    /* draw terrain mesh */
    draw_mesh();

    #ifdef GLUT
      glutSwapBuffers();
    #elif MESA_TK
      tkSwapBuffers();
    #endif
}


/* draw the terrain mesh */
static void draw_mesh() {
    glCallList(mesh);
}


/* What should we do when we have nothing else to do?  How about get
 * ready for the next move?*/
static void idle( void )
{
    slew_update();
    aircraft_debug(1);

    draw_scene();
}


/* new window size or exposure */
static void reshape( int width, int height ) {
    /* Do this so we can call reshape(0,0) ourselves without having to know
     * what the values of width & height are. */
    if ( (height > 0) && (width > 0) ) {
	win_ratio = (GLfloat) height / (GLfloat) width;
    }

    /* Inform gl of our view window size */
    glViewport(0, 0, (GLint)width, (GLint)height);

    update_view();
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


/**************************************************************************
 * Main ...
 **************************************************************************/

int main( int argc, char *argv[] ) {
    /* parse the scenery file */
    parse_scenery(argv[1]);

    #ifdef GLUT
      /* initialize GLUT */
      glutInit(&argc, argv);

      /* Define Display Parameters */
      glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

      /* Define initial window size */
      glutInitWindowSize(640, 400);

      /* Initialize the main window */
      glutCreateWindow("Terrain Demo");
    #elif MESA_TK
      /* Define initial window size */
      tkInitPosition(0, 0, 640, 400);

      /* Define Display Parameters */
      tkInitDisplayMode( TK_RGB | TK_DEPTH | TK_DOUBLE | TK_DIRECT );

      /* Initialize the main window */
      if (tkInitWindow("Terrain Demo") == GL_FALSE) {
	  tkQuit();
      }
    #endif

    /* setup view parameters, only makes GL calls */
    init_view();

    /* build all objects */
    init_scene();

    /* Set initial position and slew parameters */
    /* slew_init(-398391.3, 120070.4, 244, 3.1415); */ /* GLOBE Airport */
    slew_init(-398673.28,120625.64, 53, 4.38);

    #ifdef GLUT
      /* call reshape() on window resizes */
      glutReshapeFunc( reshape );

      /* call key() on keyboard event */
      glutKeyboardFunc( GLUTkey );
      glutSpecialFunc( GLUTkey );

      /* call idle() whenever there is nothing else to do */
      glutIdleFunc( idle );

      /* draw the scene */
      glutDisplayFunc( draw_scene );

      /* pass control off to the GLUT event handler */
      glutMainLoop();
    #elif MESA_TK
      /* call reshape() on expose events */
      tkExposeFunc( reshape );

      /* call reshape() on window resizes */
      tkReshapeFunc( reshape );

      /* call key() on keyboard event */
      tkKeyDownFunc( GLTKkey );

      /* call idle() whenever there is nothing else to do */
      tkIdleFunc( idle );

      /* draw the scene */
      tkDisplayFunc( draw_scene );

      /* pass control off to the tk event handler */
      tkExec();
    #endif

    return(0);
}


/* $Log$
/* Revision 1.3  1997/05/23 15:40:25  curt
/* Added GNU copyright headers.
/* Fog now works!
/*
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
