/**************************************************************************
 * tilecache.hxx -- routines to handle scenery tile caching
 *
 * Written by Curtis Olson, started January 1998.
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


#ifndef _TILECACHE_HXX
#define _TILECACHE_HXX


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
#include <XGL/xgl.h>

#include <Bucket/bucketutils.h>
#include <Include/fg_types.h>

/* For best results ... i.e. to avoid tile load problems and blank areas
 *
 * FG_TILE_CACHE_SIZE >= (o->tile_radius + 1) ** 2 */
#define FG_TILE_CACHE_SIZE 121


/* Tile cache record */
struct fgTILE {
    struct fgBUCKET tile_bucket;
    GLint display_list;
    fgCartesianPoint3d local_ref;
    double bounding_radius;
    int used;
    int priority;
};

/* tile cache */
extern struct fgTILE tile_cache[FG_TILE_CACHE_SIZE];


/* Initialize the tile cache subsystem */
void fgTileCacheInit( void );

/* Search for the specified "bucket" in the cache */
int fgTileCacheExists( struct fgBUCKET *p );

/* Return index of next available slot in tile cache */
int fgTileCacheNextAvail( void );

/* Fill in a tile cache entry with real data for the specified bucket */
void fgTileCacheEntryFillIn( int index, struct fgBUCKET *p );

/* Return info for a tile cache entry */
void fgTileCacheEntryInfo( int index, GLint *display_list, 
			   fgCartesianPoint3d *local_ref );


#endif /* _TILECACHE_HXX */


/* $Log$
/* Revision 1.6  1998/05/07 23:15:20  curt
/* Fixed a glTexImage2D() usage bug where width and height were mis-swapped.
/* Added support for --tile-radius=n option.
/*
 * Revision 1.5  1998/05/02 01:52:17  curt
 * Playing around with texture coordinates.
 *
 * Revision 1.4  1998/04/30 12:35:31  curt
 * Added a command line rendering option specify smooth/flat shading.
 *
 * Revision 1.3  1998/04/25 22:06:32  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.2  1998/04/24 00:51:08  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Tweaked the scenery file extentions to be "file.obj" (uncompressed)
 * or "file.obz" (compressed.)
 *
 * Revision 1.1  1998/04/22 13:22:47  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.10  1998/04/21 17:02:45  curt
 * Prepairing for C++ integration.
 *
 * Revision 1.9  1998/04/14 02:23:17  curt
 * Code reorganizations.  Added a Lib/ directory for more general libraries.
 *
 * Revision 1.8  1998/04/08 23:30:08  curt
 * Adopted Gnu automake/autoconf system.
 *
 * Revision 1.7  1998/04/03 22:11:38  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.6  1998/02/18 15:07:10  curt
 * Tweaks to build with SGI OpenGL (and therefor hopefully other accelerated
 * drivers will work.)
 *
 * Revision 1.5  1998/02/16 13:39:45  curt
 * Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
 * tiles to occasionally be missing.
 *
 * Revision 1.4  1998/01/31 00:43:27  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.3  1998/01/29 00:51:40  curt
 * First pass at tile cache, dynamic tile loading and tile unloading now works.
 *
 * Revision 1.2  1998/01/27 00:48:04  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.1  1998/01/24 00:03:29  curt
 * Initial revision.
 *
 */


