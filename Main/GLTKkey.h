/**************************************************************************
 * tkglkey.h -- handle tkgl keyboard events
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


#ifndef GLTKKEY_H
#define GLTKKEY_H


/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"

/* Handle keyboard events */
GLenum GLTKkey(int k, GLenum mask);


#endif /* GLTKKEY_H */


/* $Log$
/* Revision 1.3  1997/07/23 21:52:22  curt
/* Put comments around the text after an #endif for increased portability.
/*
 * Revision 1.2  1997/05/23 15:40:24  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.1  1997/05/21 15:57:49  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.2  1997/05/17 00:17:34  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:53  curt
 * Initial revision.
 *
 */
