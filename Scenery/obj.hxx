/**************************************************************************
 * obj.hxx -- routines to handle WaveFront .obj-ish format files.
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


#ifndef _OBJ_HXX
#define _OBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <Include/fg_types.h>

#include "tile.hxx"


/* Load a .obj file and build the GL fragment list */
int fgObjLoad(char *path, fgTILE *tile);
// GLint fgObjLoad(char *path, fgCartesianPoint3d *ref, double *radius) {


#endif /* _OBJ_HXX */


/* $Log$
/* Revision 1.3  1998/05/23 14:09:21  curt
/* Added tile.cxx and tile.hxx.
/* Working on rewriting the tile management system so a tile is just a list
/* fragments, and the fragment record contains the display list for that fragment.
/*
 * Revision 1.2  1998/05/02 01:52:15  curt
 * Playing around with texture coordinates.
 *
 * Revision 1.1  1998/04/30 12:35:29  curt
 * Added a command line rendering option specify smooth/flat shading.
 *
 * Revision 1.11  1998/04/25 22:06:31  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.10  1998/04/24 00:51:07  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Tweaked the scenery file extentions to be "file.obj" (uncompressed)
 * or "file.obz" (compressed.)
 *
 * Revision 1.9  1998/04/22 13:22:45  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.8  1998/04/21 17:02:43  curt
 * Prepairing for C++ integration.
 *
 * Revision 1.7  1998/04/03 22:11:37  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.6  1998/01/31 00:43:25  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.5  1998/01/27 00:48:03  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
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
