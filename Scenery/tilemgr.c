/**************************************************************************
 * tilemgr.c -- routines to handle dynamic management of scenery tiles
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

#include <Scenery/scenery.h>
#include <Scenery/bucketutils.h>
#include <Scenery/obj.h>
#include <Scenery/tilecache.h>

#include <Aircraft/aircraft.h>
#include <Include/constants.h>
#include <Include/types.h>


#define FG_LOCAL_X           3   /* should be odd */
#define FG_LOCAL_Y           3   /* should be odd */
#define FG_LOCAL_X_Y         9   /* At least FG_LOCAL_X times FG_LOCAL_Y */

#define FG_TILE_CACHE_SIZE 100   /* Must be > FG_LOCAL_X_Y */


/* closest (potentially viewable) tiles, centered on current tile.
 * This is an array of pointers to cache indexes. */
int tiles[FG_LOCAL_X_Y];

/* tile cache */
struct fgTILE tile_cache[FG_TILE_CACHE_SIZE];


/* Initialize the Tile Manager subsystem */
void fgTileMgrInit( void ) {
    printf("Initializing Tile Manager subsystem.\n");
    fgTileCacheInit();
}


/* given the current lon/lat, fill in the array of local chunks.  If
 * the chunk isn't already in the cache, then read it from disk. */
void fgTileMgrUpdate( void ) {
    struct fgFLIGHT *f;
    struct fgBUCKET p1, p2;
    static struct fgBUCKET p_last = {-1000, 0, 0, 0};
    int i, j, dw, dh;
    int index;

    f = &current_aircraft.flight;

    fgBucketFind(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p1);

    if ( (p1.lon == p_last.lon) && (p1.lat == p_last.lat) && 
	 (p1.x == p_last.x) && (p1.y == p_last.y) ) {
	/* same bucket as last time */
	printf("Same bucket as last time\n");
	return;
    }

    if ( p_last.lon == -1000 ) {
	printf("First time through ... \n");
	printf("Updating Tile list for %d,%d %d,%d\n", 
	       p1.lon, p1.lat, p1.x, p1.y);

	/* wipe tile cache */
	fgTileCacheInit();

	/* build the local area list and update cache */
	dw = FG_LOCAL_X / 2;
	dh = FG_LOCAL_Y / 2;

	for ( j = 0; j < FG_LOCAL_Y; j++ ) {
	    for ( i = 0; i < FG_LOCAL_X; i++ ) {
		fgBucketOffset(&p1, &p2, i - dw, j - dh);
		printf("Updating for bucket %d %d %d %d\n", 
		       p2.lon, p2.lat, p2.x, p2.y);

		index = fgTileCacheNextAvail();
		printf("Selected cache index of %d\n", index);

		tiles[(j*FG_LOCAL_Y) + i] = index;
		fgTileCacheEntryFillIn(index, &p2);
	    }
	}
    }
}


/* Render the local tiles --- hack, hack, hack */
void fgTileMgrRender( void ) {
    static GLfloat terrain_color[4] = { 0.6, 0.8, 0.4, 1.0 };
    static GLfloat terrain_ambient[4];
    static GLfloat terrain_diffuse[4];
    struct fgCartesianPoint local_ref;
    GLint display_list;
    int i;
    int index;

    for ( i = 0; i < 4; i++ ) {
	terrain_ambient[i] = terrain_color[i] * 0.5;
	terrain_diffuse[i] = terrain_color[i];
    }

    xglMaterialfv(GL_FRONT, GL_AMBIENT, terrain_ambient);
    xglMaterialfv(GL_FRONT, GL_DIFFUSE, terrain_diffuse);

    for ( i = 0; i < FG_LOCAL_X_Y; i++ ) {
	index = tiles[i];
	/* printf("Index = %d\n", index); */
	fgTileCacheEntryInfo(index, &display_list, &local_ref );

	xglPushMatrix();
	xglTranslatef(local_ref.x - scenery.center.x,
		      local_ref.y - scenery.center.y,
		      local_ref.z - scenery.center.z);
	xglCallList(display_list);
	xglPopMatrix();
    }
}


/* $Log$
/* Revision 1.6  1998/01/24 00:03:30  curt
/* Initial revision.
/*
 * Revision 1.5  1998/01/19 19:27:18  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.4  1998/01/19 18:40:38  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.3  1998/01/13 00:23:11  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.2  1998/01/08 02:22:27  curt
 * Continue working on basic features.
 *
 * Revision 1.1  1998/01/07 23:50:51  curt
 * "area" renamed to "tile"
 *
 * Revision 1.2  1998/01/07 03:29:29  curt
 * Given an arbitrary lat/lon, we can now:
 *   generate a unique index for the chunk containing the lat/lon
 *   generate a path name to the chunk file
 *   build a list of the indexes of all the nearby areas.
 *
 * Revision 1.1  1998/01/07 02:05:48  curt
 * Initial revision.
 * */


