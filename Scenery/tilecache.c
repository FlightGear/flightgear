/**************************************************************************
 * tilecache.c -- routines to hancle scenery tile caching
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


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Scenery/bucketutils.h>
#include <Scenery/obj.h>
#include <Scenery/tilecache.h>

#include <Include/general.h>

/* tile cache */
struct fgTILE tile_cache[FG_TILE_CACHE_SIZE];


/* Initialize the tile cache subsystem */
void fgTileCacheInit( void ) {
    int i;

    printf("Initializing the tile cache.\n");

    for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	tile_cache[i].used = 0;
    }
}


/* Return index of next available slot in tile cache */
int fgTileCacheNextAvail( void ) {
     int i;
     
     for ( i = 0; i < FG_TILE_CACHE_SIZE; i++ ) {
	 if ( tile_cache[i].used == 0 ) {
	     return(i);
	 }
     }

     return(-1);
}


/* Fill in a tile cache entry with real data for the specified bucket */
void fgTileCacheEntryFillIn( int index, struct fgBUCKET *p ) {
    struct fgGENERAL *g;
    char base_path[256];
    char file_name[256];

    g = &general;

    /* Mark this cache entry as used */
    tile_cache[index].used = 1;

    /* Update the bucket */
    tile_cache[index].tile_bucket.lon = p->lon;
    tile_cache[index].tile_bucket.lat = p->lat;
    tile_cache[index].tile_bucket.x = p->x;
    tile_cache[index].tile_bucket.y = p->y;

    /* Load the appropriate area and get the display list pointer */
    fgBucketGenBasePath(p, base_path);
    sprintf(file_name, "%s/Scenery/%s/%ld.obj", g->root_dir, 
	    base_path, fgBucketGenIndex(p));
    tile_cache[index].display_list = 
	fgObjLoad(file_name, &tile_cache[index].local_ref);    
}


/* Return info for a tile cache entry */
void fgTileCacheEntryInfo( int index, GLint *display_list, 
			   struct fgCartesianPoint *local_ref ) {
    *display_list = tile_cache[index].display_list;
    /* printf("Display list = %d\n", *display_list); */

    local_ref->x = tile_cache[index].local_ref.x;
    local_ref->y = tile_cache[index].local_ref.y;
    local_ref->z = tile_cache[index].local_ref.z;
}


/* Free the specified cache entry
void fgTileCacheEntryFree( in index ) {
}
*/


/* $Log$
/* Revision 1.2  1998/01/26 15:55:24  curt
/* Progressing on building dynamic scenery system.
/*
 * Revision 1.1  1998/01/24 00:03:29  curt
 * Initial revision.
 *
 */


