// tilecache.cxx -- routines to handle scenery tile caching
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Bucket/bucketutils.h>
#include <Debug/fg_debug.h>
#include <Main/options.hxx>
#include <Main/views.hxx>

#include "obj.hxx"
#include "tilecache.hxx"


// the tile cache
fgTILECACHE global_tile_cache;


// Constructor
fgTILECACHE::fgTILECACHE( void ) {
}


// Initialize the tile cache subsystem
void fgTILECACHE::Init( void ) {
    int i;

    fgPrintf(FG_TERRAIN, FG_INFO, "Initializing the tile cache.\n");

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	tile_cache[i].used = 0;
    }
}


// Search for the specified "bucket" in the cache
int fgTILECACHE::Exists( struct fgBUCKET *p ) {
    int i;

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	if ( tile_cache[i].tile_bucket.lon == p->lon ) {
	    if ( tile_cache[i].tile_bucket.lat == p->lat ) {
		if ( tile_cache[i].tile_bucket.x == p->x ) {
		    if ( tile_cache[i].tile_bucket.y == p->y ) {
			fgPrintf( FG_TERRAIN, FG_DEBUG, 
				  "TILE EXISTS in cache ... index = %d\n", i );
			return( i );
		    }
		}
	    }
	}
    }
    
    return( -1 );
}


// Fill in a tile cache entry with real data for the specified bucket
void fgTILECACHE::EntryFillIn( int index, struct fgBUCKET *p ) {
    fgOPTIONS *o;
    char base_path[256];
    char file_name[256];

    o = &current_options;

    // Mark this cache entry as used
    tile_cache[index].used = 1;

    // Update the bucket
    tile_cache[index].tile_bucket.lon = p->lon;
    tile_cache[index].tile_bucket.lat = p->lat;
    tile_cache[index].tile_bucket.x = p->x;
    tile_cache[index].tile_bucket.y = p->y;

    // Load the appropriate data file and built tile fragment list
    fgBucketGenBasePath(p, base_path);
    sprintf(file_name, "%s/Scenery/%s/%ld", o->fg_root, 
	    base_path, fgBucketGenIndex(p));
    fgObjLoad(file_name, &tile_cache[index]);
    /*
    tile_cache[index].display_list = 
	fgObjLoad(file_name, &tile_cache[index].local_ref,
		  &tile_cache[index].bounding_radius);    
		  */
}


// Free a tile cache entry
void fgTILECACHE::EntryFree( int index ) {
    fgFRAGMENT fragment;

    // Mark this cache entry as un-used
    tile_cache[index].used = 0;

    // Update the bucket
    fgPrintf( FG_TERRAIN, FG_DEBUG, 
	      "FREEING TILE = (%d %d %d %d)\n",
	      tile_cache[index].tile_bucket.lon, 
	      tile_cache[index].tile_bucket.lat, 
	      tile_cache[index].tile_bucket.x,
	      tile_cache[index].tile_bucket.y );

    // Step through the fragment list, deleting the display list, then
    // the fragment, until the list is empty.
    while ( tile_cache[index].fragment_list.size() ) {
	fragment = tile_cache[index].fragment_list.front();
	xglDeleteLists( fragment.display_list, 1 );
	tile_cache[index].fragment_list.pop_front();
    }
}


// Return the specified tile cache entry 
fgTILE *fgTILECACHE::GetTile( int index ) {
    return ( &tile_cache[index] );
}


// Return index of next available slot in tile cache
int fgTILECACHE::NextAvail( void ) {
    fgVIEW *v;
    int i;
    float dx, dy, dz, max, med, min, tmp;
    float dist, max_dist;
    int max_index;
    
    v = &current_view;

    max_dist = 0.0;
    max_index = 0;

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	if ( tile_cache[i].used == 0 ) {
	    return(i);
	} else {
	    // calculate approximate distance from view point
	    fgPrintf( FG_TERRAIN, FG_DEBUG,
		      "DIST Abs view pos = %.4f, %.4f, %.4f\n", 
		      v->abs_view_pos.x, v->abs_view_pos.y, v->abs_view_pos.z );
	    fgPrintf( FG_TERRAIN, FG_DEBUG,
		      "    ref point = %.4f, %.4f, %.4f\n", 
		      tile_cache[i].center.x, tile_cache[i].center.y,
		      tile_cache[i].center.z);

	    dx = fabs(tile_cache[i].center.x - v->abs_view_pos.x);
	    dy = fabs(tile_cache[i].center.y - v->abs_view_pos.y);
	    dz = fabs(tile_cache[i].center.z - v->abs_view_pos.z);

	    max = dx; med = dy; min = dz;
	    if ( max < med ) {
		tmp = max; max = med; med = tmp;
	    }
	    if ( max < min ) {
		tmp = max; max = min; min = tmp;
	    }
	    dist = max + (med + min) / 4;

	    fgPrintf( FG_TERRAIN, FG_DEBUG, "    distance = %.2f\n", dist);

	    if ( dist > max_dist ) {
		max_dist = dist;
		max_index = i;
	    }
	}
    }

    // If we made it this far, then there were no open cache entries.
    // We will instead free the furthest cache entry and return it's
    // index.
    
    EntryFree( max_index );
    return( max_index );
}


// Destructor
fgTILECACHE::~fgTILECACHE( void ) {
}


// $Log$
// Revision 1.10  1998/05/23 14:09:22  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
// Revision 1.9  1998/05/20 20:53:54  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.8  1998/05/16 13:09:57  curt
// Beginning to add support for view frustum culling.
// Added some temporary code to calculate bouding radius, until the
//   scenery generation tools and scenery can be updated.
//
// Revision 1.7  1998/05/13 18:26:41  curt
// Root path info moved to fgOPTIONS.
//
// Revision 1.6  1998/05/02 01:52:17  curt
// Playing around with texture coordinates.
//
// Revision 1.5  1998/04/30 12:35:31  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.4  1998/04/28 01:21:43  curt
// Tweaked texture parameter calculations to keep the number smaller.  This
// avoids the "swimming" problem.
// Type-ified fgTIME and fgVIEW.
//
// Revision 1.3  1998/04/25 22:06:32  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/24 00:51:08  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.1  1998/04/22 13:22:46  curt
// C++ - ifing the code a bit.
//
// Revision 1.11  1998/04/18 04:14:07  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.10  1998/04/14 02:23:17  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.9  1998/04/08 23:30:07  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.8  1998/04/03 22:11:38  curt
// Converting to Gnu autoconf system.
//
// Revision 1.7  1998/02/01 03:39:55  curt
// Minor tweaks.
//
// Revision 1.6  1998/01/31 00:43:26  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.5  1998/01/29 00:51:39  curt
// First pass at tile cache, dynamic tile loading and tile unloading now works.
//
// Revision 1.4  1998/01/27 03:26:43  curt
// Playing with new fgPrintf command.
//
// Revision 1.3  1998/01/27 00:48:03  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.2  1998/01/26 15:55:24  curt
// Progressing on building dynamic scenery system.
//
// Revision 1.1  1998/01/24 00:03:29  curt
// Initial revision.



