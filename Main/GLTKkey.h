/**************************************************************************
 * tkglkey.h -- handle tkgl keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef GLTKKEY_H
#define GLTKKEY_H


/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"

/* Handle keyboard events */
GLenum GLTKkey(int k, GLenum mask);


#endif GLTKKEY_H


/* $Log$
/* Revision 1.1  1997/05/21 15:57:49  curt
/* Renamed due to added GLUT support.
/*
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:53  curt
 * Initial revision.
 *
 */
