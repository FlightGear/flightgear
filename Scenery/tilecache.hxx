// tilecache.hxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


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

#include "tile.hxx"


// For best results ... i.e. to avoid tile load problems and blank areas
//
// FG_TILE_CACHE_SIZE >= (o->tile_diameter + 1) ** 2 
#define FG_TILE_CACHE_SIZE 121


/*
// Tile cache record 
typedef struct {
    fgBUCKET tile_bucket;
    GLint display_list;
    fgCartesianPoint3d local_ref;
    double bounding_radius;
    int used;
    int priority;
} fgTILE;
*/


// A class to store and manage a pile of tiles
class fgTILECACHE {
    // cache storage space
    fgTILE tile_cache[FG_TILE_CACHE_SIZE];

public:

    // Constructor
    fgTILECACHE( void );

    // Initialize the tile cache subsystem 
    void init( void );

    // Search for the specified "bucket" in the cache 
    int exists( fgBUCKET *p );

    // Return index of next available slot in tile cache 
    int next_avail( void );

    // Free a tile cache entry
    void entry_free( int index );

    // Fill in a tile cache entry with real data for the specified bucket 
    void fill_in( int index, fgBUCKET *p );

    // Return a pointer to the specified tile cache entry 
    fgTILE *get_tile( int index );

    // Destructor
    ~fgTILECACHE( void );
};


// the tile cache
extern fgTILECACHE global_tile_cache;


#endif // _TILECACHE_HXX 


// $Log$
// Revision 1.11  1998/09/14 12:45:25  curt
// minor tweaks.
//
// Revision 1.10  1998/07/04 00:54:31  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.9  1998/05/23 14:09:22  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
// Revision 1.8  1998/05/20 20:53:54  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.7  1998/05/16 13:09:57  curt
// Beginning to add support for view frustum culling.
// Added some temporary code to calculate bouding radius, until the
//   scenery generation tools and scenery can be updated.
//
// Revision 1.6  1998/05/07 23:15:20  curt
// Fixed a glTexImage2D() usage bug where width and height were mis-swapped.
// Added support for --tile-radius=n option.
//
// Revision 1.5  1998/05/02 01:52:17  curt
// Playing around with texture coordinates.
//
// Revision 1.4  1998/04/30 12:35:31  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.3  1998/04/25 22:06:32  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/24 00:51:08  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.1  1998/04/22 13:22:47  curt
// C++ - ifing the code a bit.
//
// Revision 1.10  1998/04/21 17:02:45  curt
// Prepairing for C++ integration.
//
// Revision 1.9  1998/04/14 02:23:17  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.8  1998/04/08 23:30:08  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.7  1998/04/03 22:11:38  curt
// Converting to Gnu autoconf system.
//
// Revision 1.6  1998/02/18 15:07:10  curt
// Tweaks to build with SGI OpenGL (and therefor hopefully other accelerated
// drivers will work.)
//
// Revision 1.5  1998/02/16 13:39:45  curt
// Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
// tiles to occasionally be missing.
//
// Revision 1.4  1998/01/31 00:43:27  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.3  1998/01/29 00:51:40  curt
// First pass at tile cache, dynamic tile loading and tile unloading now works.
//
// Revision 1.2  1998/01/27 00:48:04  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.1  1998/01/24 00:03:29  curt
// Initial revision.


