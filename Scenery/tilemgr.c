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
#include "../XGL/xgl.h"

#include "scenery.h"
#include "tileutils.h"
#include "obj.h"

#include "../Aircraft/aircraft.h"
#include "../Include/constants.h"
#include "../Include/general.h"
#include "../Include/types.h"


/* here's where we keep the array of closest (potentially viewable) tiles */
struct bucket local_tiles[49];
GLint local_display_lists[49];
struct fgCartesianPoint local_refs[49];


/* Initialize the Tile Manager subsystem */
void fgTileMgrInit( void ) {
    printf("Initializing Tile Manager subsystem.\n");
    /* fgTileCacheInit(); */
}


/* given the current lon/lat, fill in the array of local chunks.  If
 * the chunk isn't already in the cache, then read it from disk. */
void fgTileMgrUpdate( void ) {
    struct fgFLIGHT *f;
    struct fgGENERAL *g;
    struct bucket p;
    struct bucket p_last = {-1000, 0, 0, 0};
    char base_path[256];
    char file_name[256];
    int i, j;

    f = &current_aircraft.flight;
    g = &general;

    find_bucket(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p);
    printf("Updating Tile list for %d,%d %d,%d\n", p.lon, p.lat, p.x, p.y);

    if ( (p.lon == p_last.lon) && (p.lat == p_last.lat) && 
	 (p.x == p_last.x) && (p.y == p_last.y) ) {
	/* same bucket as last time */
    }

    gen_idx_array(&p, local_tiles, 7, 7);

    /* scenery.center = ref; */

    for ( i = 0; i < 49; i++ ) {
	gen_base_path(&local_tiles[i], base_path);
	sprintf(file_name, "%s/Scenery/%s/%ld.obj", 
		g->root_dir, base_path, gen_index(&local_tiles[i]));
	local_display_lists[i] = 
	    fgObjLoad(file_name, &local_refs[i]);

	if ( (local_tiles[i].lon == p.lon) &&
	     (local_tiles[i].lat == p.lat) &&
	     (local_tiles[i].x == p.x) &&
	     (local_tiles[i].y == p.y) ) {
	    scenery.center = local_refs[i];
	}
    }
}


/* Render the local tiles --- hack, hack, hack */
void fgTileMgrRender( void ) {
    static GLfloat terrain_color[4] = { 0.6, 0.8, 0.4, 1.0 };
    static GLfloat terrain_ambient[4];
    static GLfloat terrain_diffuse[4];
    int i, j;

    for ( i = 0; i < 4; i++ ) {
	terrain_ambient[i] = terrain_color[i] * 0.5;
	terrain_diffuse[i] = terrain_color[i];
    }

    xglMaterialfv(GL_FRONT, GL_AMBIENT, terrain_ambient);
    xglMaterialfv(GL_FRONT, GL_DIFFUSE, terrain_diffuse);

    for ( i = 0; i < 49; i++ ) {
	xglPushMatrix();
	xglTranslatef(local_refs[i].x - scenery.center.x,
		      local_refs[i].y - scenery.center.y,
		      local_refs[i].z - scenery.center.z);
	xglCallList(local_display_lists[i]);
	xglPopMatrix();
    }
}


/* $Log$
/* Revision 1.4  1998/01/19 18:40:38  curt
/* Tons of little changes to clean up the code and to remove fatal errors
/* when building with the c++ compiler.
/*
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


