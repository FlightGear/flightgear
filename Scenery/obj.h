/**************************************************************************
 * obj.h -- routines to handle WaveFront .obj format files.
 *
 * Written by Curtis Olson, started October 1997.
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


#ifndef _OBJ_H
#define _OBJ_H


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <Include/fg_types.h>


/* Load a .obj file and generate the GL call list */
GLint fgObjLoad(char *file, struct fgCartesianPoint *ref);


#endif /* _OBJ_H */


/* $Log$
/* Revision 1.5  1998/01/27 00:48:03  curt
/* Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
/* system and commandline/config file processing code.
/*
 * Revision 1.4  1998/01/22 02:59:41  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.3  1998/01/19 19:27:17  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/13 00:23:10  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.1  1997/10/28 21:14:55  curt
 * Initial revision.
 *
 */
