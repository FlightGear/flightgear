/**************************************************************************
 * GLUTkey.h -- handle GLUT keyboard events
 *
 * Written by Curtis Olson, started May 1997.
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
void GLUTspecialkey(int k, int x, int y);


#endif GLUTKEY_H


/* $Log$
/* Revision 1.4  1997/06/02 03:40:06  curt
/* A tiny bit more view tweaking.
/*
 * Revision 1.3  1997/05/31 04:13:52  curt
 * WE CAN NOW FLY!!!
 *
 * Continuing work on the LaRCsim flight model integration.
 * Added some MSFS-like keyboard input handling.
 *
 * Revision 1.2  1997/05/23 15:40:25  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.1  1997/05/21 15:57:51  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:53  curt
 * Initial revision.
 *
 */
