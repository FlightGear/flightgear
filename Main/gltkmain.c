/**************************************************************************
 * gltkmain.c -- top level sim routines
 *
 * Written by Curtis Olson for Mesa (OpenGL), started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef MESA
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
#else
    #include <GL/gl.h>
/* #include "aux.h" */
#endif

#include "gltkkey.h"
#include "../aircraft/aircraft.h"
#include "../scenery/scenery.h"


/* This a record containing all the info for the aircraft currently
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

/* init_view() -- Setup view parameters */
static void init_view() {
    /* if the 4th field is 0.0, this specifies a direction ... */
    static GLfloat pos[4] = {-3.0, 1.0, 3.0, 0.0 };
    static GLfloat fogColor[4] = {0.5, 0.5, 0.5, 1.0};
    static GLfloat local_view[] = { 0.0 };
    
    glLightfv( GL_LIGHT0, GL_POSITION, pos );
    glEnable( GL_CULL_FACE );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
/**/glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);
    glEnable( GL_DEPTH_TEST );
/**/glDepthFunc(GL_LEQUAL);

/*  glEnable( GL_FOG );
    glFogi (GL_FOG_MODE, GL_LINEAR);
    glFogf (GL_FOG_START, 1.0);
    glFogf (GL_FOG_END, 100.0);
    glFogfv (GL_FOG_COLOR, fogColor);
    glFogf (GL_FOG_DENSITY, 0.35);
    glHint (GL_FOG_HINT, GL_DONT_CARE);
    */
    
    glClearColor(0.6, 0.6, 0.9, 1.0);
}


/* init_scene() -- build all objects */
static void init_scene() {

    /* make terrain mesh */
    mesh = make_mesh();

    /* If enabled, normal vectors specified with glNormal are scaled
       to unit length after transformation.  See glNormal. */
    glEnable( GL_NORMALIZE );
}


/* create the terrain mesh */
GLint make_mesh() {
    GLint mesh;

    mesh = mesh_to_ogl(mesh_ptr);

    return(mesh);
}


/* create the terrain mesh */
GLint make_mesh_old() {
    GLint mesh;
    static GLfloat color[4] = { 0.3, 0.7, 0.2, 1.0 };

    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color );
    glShadeModel( GL_FLAT ); /*  glShadeModel( GL_SMOOTH ); */

    glBegin(GL_POLYGON);
        glVertex3f(-10.0, -10.0, 0.0);
	glVertex3f(0.0, -10.0, 0.0);
	glVertex3f(0.0, 0.0, 1.0);
	glVertex3f(-10.0, 0.0, 1.0);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(-10.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 10.0, 0.0);
	glVertex3f(-10.0, 10.0, 0.0);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(10.0, 0.0, 2.0);
	glVertex3f(10.0, 10.0, 2.0);
	glVertex3f(0.0, 10.0, 0.0);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, -10.0, -1.0);
	glVertex3f(10.0, -10.0, 0.0);
	glVertex3f(10.0, 0.0, -1.0);
	glVertex3f(0.0, 0.0, 0.0);
    glEnd();

    glEndList();

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
    gluLookAt(f->pos_x, f->pos_y, f->pos_z,
	      f->pos_x + cos(f->Psi), f->pos_y + sin(f->Psi), f->pos_z,
	      0.0, 0.0, 1.0);
}


/* draw the scene */
static void draw_scene( void ) {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* update view volume parameters */
    update_view();

    /* Tell GL we are switching to model view parameters */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* glTranslatef(0.0, 0.0, -5.0); */

    glPushMatrix();

    /* draw terrain mesh */
    draw_mesh();

    glPopMatrix();

    tkSwapBuffers();
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

    /* Define initial window size */
    tkInitPosition(0, 0, 640, 400);

    /* Define Display Parameters */
    tkInitDisplayMode( TK_RGB | TK_DEPTH | TK_DOUBLE | TK_DIRECT );

    /* Initialize the main window */
    if (tkInitWindow("Terrain Demo") == GL_FALSE) {
	tkQuit();
    }

    /* setup view parameters */
    init_view();

    /* build all objects */
    init_scene();

    /* Set initial position and slew parameters */
    /* slew_init(-398391.3, 120070.4, 244, 3.1415); */ /* GLOBE Airport */
    slew_init(-398673.28,120625.64, 53, 4.38);

    /* call reshape() on expose events */
    tkExposeFunc( reshape );

    /* call reshape() on window resizes */
    tkReshapeFunc( reshape );

    /* call key() on keyboard event */
    tkKeyDownFunc( key );

    /* call idle() whenever there is nothing else to do */
    tkIdleFunc( idle );

    /* draw the scene */
    tkDisplayFunc( draw_scene );

    /* pass control off to the tk event handler */
    tkExec();

    return(0);
}


/* $Log$
/* Revision 1.3  1997/05/19 18:22:42  curt
/* Parameter tweaking ... starting to stub in fog support.
/*
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:52  curt
 * Initial revision.
 *
 */
