/**************************************************************************
 * GLUTkey.h -- handle GLUT keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef GLUTKEY_H
#define GLUTKEY_H


#ifdef GLUT
    #include <GL/glut.h>
#elif TIGER
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
#endif

/* Handle keyboard events */
void GLUTkey(unsigned char k, int x, int y);


#endif GLUTKEY_H


/* $Log$
/* Revision 1.1  1997/05/21 15:57:51  curt
/* Renamed due to added GLUT support.
/*
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:53  curt
 * Initial revision.
 *
 */
